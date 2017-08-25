#include <thread>
#include <sstream>
#include <cstring>
#include <unordered_map>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <unistd.h>
#include <fcntl.h>

#include <common/Profiler.h>

#include "StateMachine.h"
#include "HttpServer.h"

namespace http
{
static const char* RESPONSE_200[2] = 
{
  "HTTP/1.1 200 OK\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: close\n"
  "Content-Length: 2\n"
  "\n"
  "{}",
  "HTTP/1.1 200 OK\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: keep-alive\n"
  "Content-Length: 2\n"
  "\n"
  "{}",
};
static const size_t RESPONSE_200_SIZE[] = { 101, 106 };

static const char* RESPONSE_400[2] = 
{
  "HTTP/1.1 400 Bad Request\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: close\n"
  "Content-Length: 2\n"
  "\n"
  "{}",
  "HTTP/1.1 400 Bad Request\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: keep-alive\n"
  "Content-Length: 2\n"
  "\n"
  "{}"
};

static const size_t RESPONSE_400_SIZE[] = { 110, 115 };

static const char* RESPONSE_404[2] = 
{
  "HTTP/1.1 404 Not Found\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: close\n"
  "Content-Length: 2\n"
  "\n"
  "{}",
  "HTTP/1.1 404 Not Found\n"
  "Content-Type: application/json; charset=UTF-8\n"
  "Connection: keep-alive\n"
  "Content-Length: 2\n"
  "\n"
  "{}"
};
static const size_t RESPONSE_404_SIZE[] = { 108, 113 };

#define RESPONSE_200_PART1 \
  "HTTP/1.1 200 OK\n"\
  "Content-Type: application/json; charset=UTF-8\n"\
  "Connection: close\n"\
  "Content-Length: "
#define RESPONSE_200_PART1_SIZE 96
#define RESPONSE_200_PART1_KEEPALIVE \
  "HTTP/1.1 200 OK\n"\
  "Content-Type: application/json; charset=UTF-8\n"\
  "Connection: keep-alive\n"\
  "Content-Length: "
#define RESPONSE_200_PART1_KEEPALIVE_SIZE 101

#define RESPONSE_200_PART2 \
  "%zu\n"\
  "\n"

HttpServer::HttpServer(db::StoragePtr& storage):
  tcp::TcpServer(),
  storage_(storage),
  threadPool_(THREADS_COUNT, std::bind(&HttpServer::handleRequest, this, std::placeholders::_1))
{}

HttpServer::~HttpServer()
{}

HTTPCode HttpServer::parseURL(Request& req, char* url, int32_t urlSize)
{
  static std::unordered_map<std::string, Table> strToTableMap = 
  {
    { "users",     Table::USERS },
    { "locations", Table::LOCATIONS },
    { "visits",    Table::VISITS },
    { "avg",       Table::AVG },
  };

  LOG(stderr, "URL: %s\n", url);

  if (urlSize <= 0) {
    return HTTPCode::BAD_REQ;
  }

  enum class State
  {
    TABLE1,
    ID,
    TABLE2,
    ERROR,
  } state = State::TABLE1;

  char* next = strchr(url + 1, '?');
  if (next) {
    *next = '\0';
    req.params_ = next + 1;
    req.paramsSize_ = urlSize - (req.params_ - url);
    urlSize -= (req.paramsSize_ + 1); 
  }

  char* prev = nullptr;
  next = url;
  do
  {
    prev = next + 1;
    next = strchr(prev, '/');
    int size;
    if (!next) {
      size = urlSize - (prev - url);
    } else {
      *next = '\0';
      size = next - prev;
    }

    switch (state) {
      case State::TABLE1:
      case State::TABLE2:
      {
        //LOG(stderr, "Searching for table: %s\n", prev);
        auto it = strToTableMap.find(std::string(prev, size));
        if (strToTableMap.end() == it) {
          return HTTPCode::BAD_REQ;
        }
        if (State::TABLE1 == state) {
          LOG(stderr, "TABLE 1: %s\n", prev);
          req.table1_ = it->second;
          state = State::ID;
        } else {
          LOG(stderr, "TABLE 2: %s\n", prev);
          req.table2_ = it->second;
          state = State::ERROR;
        }
        break;
      }
      case State::ID:
      {
        LOG(stderr, "ID: %s\n", prev);
        if (0 == strncmp(prev, "new", size)) {
          req.id_ = -1;
        } else {
          char* end;
          req.id_ = static_cast<int32_t>(strtol(prev, &end, 10));
          if (!end) {
            return HTTPCode::NOT_FOUND;
          }
        }
        state = State::TABLE2;
        break;
      }
      case State::ERROR:
        //LOG(stderr, "Error state\n");
        return HTTPCode::BAD_REQ;
    }
    //LOG(stderr, "New state = %d\n", state);

  } while (next);

  return HTTPCode::OK;
}

HTTPCode HttpServer::parseRequestMethod(Request& req, char* reqMethod, int32_t size)
{
  if (size <= 0) {
    return HTTPCode::BAD_REQ;
  }
  char* next = strchr(reqMethod, ' ');
  if (!next) {
    return HTTPCode::BAD_REQ;
  }
  *next = '\0';
  if (0 == strncmp(reqMethod, "POST", next - reqMethod)) {
    req.type_ = Type::POST;
  } else if (0 == strncmp(reqMethod, "GET", next - reqMethod)) {
    req.type_ = Type::GET;
  } else {
    return HTTPCode::BAD_REQ;
  }
  reqMethod = next + 1;
  next = strchr(reqMethod, ' ');
  if (!next) {
    return HTTPCode::BAD_REQ;
  }
  *next = '\0';
  HTTPCode code = parseURL(req, reqMethod, next - reqMethod);
  if (HTTPCode::OK != code) {
    LOG(stderr, "Parsing URL error code = %d\n", code);
    return code;
  }

  return HTTPCode::OK;
}

HTTPCode HttpServer::parseHeader(Request& req, bool& hasNext, char* header, int32_t size)
{
  if (size <= 0) {
    return HTTPCode::BAD_REQ;
  }
  if (size == 1 && *header == '\r') {
    hasNext = false;
    return HTTPCode::OK;
  }
  if (*header != 'C') {
    hasNext = true;
    return HTTPCode::OK;
  }
  //LOG(stderr, "HEADER: %s\n", header);
  if (0 == strncmp(header, "Content", 7)) {
    if (0 == strncmp(header + 7, "-Length", 7)) {
      char* val = strchr(header + 14, ':');
      if (!val) {
        return HTTPCode::BAD_REQ;
      }
      char* end;
      req.contentLength_ = static_cast<int32_t>(strtol(val + 1, &end, 10));
      if (!end) {
        return HTTPCode::NOT_FOUND;
      }
    } else if (0 == strncmp(header + 7, "-Type", 5)) {
      req.hasContentType_ = true;
    }
  }
  hasNext = true;
  return HTTPCode::OK;
}

void HttpServer::handleRequest(tcp::SocketWrapper sock)
{
  tcp::Socket _sock(sock);

  bool keepalive = false;
  do {
    Request req;
    HTTPCode code;
    {
      code = readRequest(req, _sock);
    }
    if (HTTPCode::NO_ERROR == code) {
      return;
    }
    keepalive = req.keepalive_;
    if (HTTPCode::OK != code) {
      sendResponse(_sock, code, keepalive);
      continue;
    }

    StateMachine::Handler handler = StateMachine::getHandler(req);
    if (!handler) {
      sendResponse(_sock, code, keepalive);
      continue;
    }

    Response resp;
    {
      START_PROFILER("handler");
      code = handler(resp, *storage_, req);
      STOP_PROFILER;
    }
    if (HTTPCode::OK != code) {
      sendResponse(_sock, code, keepalive);
      continue;
    }

    if (Type::POST == req.type_) {
      sendResponse(_sock, HTTPCode::OK, keepalive);
    } else {
      sendResponse(_sock, resp, keepalive);
    }
  } while (keepalive);
}

HTTPCode HttpServer::readRequest(Request& req, tcp::SocketWrapper sock)
{
  enum class State
  {
    METHOD,
    HEADERS,
    BODY,
  } state = State::METHOD;
  HTTPCode code = HTTPCode::NO_ERROR;
  thread_local char buffer[4 * 1024];
  int offset = 0, size = 0;
  int res;
  do {
    res = sock.recv(buffer + size, sizeof(buffer) - size);
    LOG(stderr, "res = %d\n", res);
    if (-1 == res) {
      if (EAGAIN == errno || EWOULDBLOCK == errno) {
        res = 0;
        continue;
      }
    }
    if (res > 0) {
      size += res;
      buffer[size] = '\0';
      LOG(stderr, "Received buffer %s\n", buffer);
      if (State::BODY == state) {
        if (size - offset < req.contentLength_) {
          continue;
        }
        req.content_ = buffer + offset;
        return HTTPCode::OK;
      }
      char* next = strchr(buffer + offset, '\n');
      while (next && offset < size) {
        *next = '\0';
        switch (state) {
          case State::METHOD:
          {
            START_PROFILER("parseRequestMethod");
            code = parseRequestMethod(req, buffer + offset, next - buffer - offset);
            STOP_PROFILER;
            if (HTTPCode::OK != code) {
              LOG(stderr, "Method : %s. Code : %d\n", buffer + offset, code);
              return code;
            }
            if (Type::GET == req.type_) {
              req.keepalive_ = true;
              return HTTPCode::OK;
            }
            state = State::HEADERS;
            break;
          }
          case State::HEADERS:
          {
            START_PROFILER("parseHeader");
            bool hasNext;
            code = parseHeader(req, hasNext, buffer + offset, next - buffer - offset);
            STOP_PROFILER;
            if (HTTPCode::OK != code) {
              LOG(stderr, "Header : %s. Code : %d\n", buffer + offset, code);
              return code;
            }
            if (!hasNext) {
              state = State::BODY;
            }
            break;
          }
          default:
            break;
        }
        offset = next - buffer + 1;
        if (State::BODY == state) {
          if (size - offset < req.contentLength_) {
            break;
          }
          req.content_ = buffer + offset;
          return HTTPCode::OK;
        }
        next = strchr(buffer + offset, '\n');
      }
    }
  } while (res > 0);
  return code;
}

void HttpServer::sendResponse(tcp::SocketWrapper sock, const HTTPCode code, bool keepalive)
{
  //int* outPipe = outPipes_[threadIdx_.fetch_add(1) % THREADS_COUNT];

  size_t idx = keepalive ? 1 : 0;
  switch (code) {
    case HTTPCode::OK:
    {
      /*LOG(stderr, "200: tee\n");
      ssize_t res = tee(pipe200_[0], outPipe[1], RESPONSE_200_SIZE, 0);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 200 code\n");
        return ;
      }
      LOG(stderr, "200: %zd bytes have been teed\n", res);
      res = splice(outPipe[0], nullptr, int(sock), nullptr, RESPONSE_200_SIZE, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 200 code\n");
        return ;
      }
      LOG(stderr, "200: %zd bytes have been spliced\n", res);
      */
      send(sock, RESPONSE_200[idx], RESPONSE_200_SIZE[idx]);
      break;
    }
    case HTTPCode::BAD_REQ:
    {
      /*LOG(stderr, "400: tee\n");
      ssize_t res = tee(pipe400_[0], outPipe[1], RESPONSE_400_SIZE, 0);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 400 code\n");
        return ;
      }
      LOG(stderr, "400: %zd bytes have been teed\n", res);
      res = splice(outPipe[0], nullptr, int(sock), nullptr, RESPONSE_400_SIZE, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 400 code\n");
        return ;
      }
      LOG(stderr, "400: %zd bytes have been spliced\n", res);
      */
      send(sock, RESPONSE_400[idx], RESPONSE_400_SIZE[idx]);
      break;
    }
    case HTTPCode::NOT_FOUND:
    {
      /*
      LOG(stderr, "404: tee\n");
      ssize_t res = tee(pipe404_[0], outPipe[1], RESPONSE_404_SIZE, 0);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 404 code\n");
        return ;
      }
      LOG(stderr, "404: %zd bytes have been teed\n", res);
      res = splice(outPipe[0], nullptr, int(sock), nullptr, RESPONSE_404_SIZE, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
      if (-1 == res) {
        LOG_CRITICAL(stderr, "Cannot send response with 404 code\n");
        return ;
      }
      LOG(stderr, "404: %zd bytes have been spliced\n", res);
      */
      send(sock, RESPONSE_404[idx], RESPONSE_404_SIZE[idx]);
      break;
    }
    default:
      break;
  }
}

void HttpServer::sendResponse(tcp::SocketWrapper sock, const Response& resp, bool keepalive)
{
  const char* buffer;
  int size;
  if (resp.buffer.size != 0) {
    buffer = resp.buffer.buffer;
    size = resp.buffer.size;
  } else {
    buffer = resp.constBuffer.buffer;
    size = resp.constBuffer.size;
  }
  send(sock, buffer, size);
}

void HttpServer::write(int fd, const char* buffer, int size)
{
  if (size <= 0) {
    return ;
  }

  LOG(stderr, "Writing buffer: %s\n", buffer);

  int offset = 0;
  while (offset < size) {
    int sent = ::write(fd, buffer + offset, size - offset);
    if (sent < 0) {
      LOG_CRITICAL(stderr, "Write errno = %s(%d)\n", std::strerror(errno), errno);
      break;
    }
    offset += sent;
  }
}

void HttpServer::send(tcp::SocketWrapper sock, const char* buffer, int size)
{
  if (size <= 0) {
    return ;
  }

  LOG(stderr, "Sending buffer: %s\n", buffer);

  int offset = 0;
  while (offset < size) {
    int sent = sock.send(buffer + offset, size - offset, MSG_DONTWAIT);
    if (sent < 0) {
      LOG_CRITICAL(stderr, "Send errno = %s(%d)\n", std::strerror(errno), errno);
      break;
    }
    offset += sent;
  }
}

Result HttpServer::doStart()
{
  return Result::SUCCESS;
#if 0
  pipe2(pipe200_, O_NONBLOCK);
  pipe2(pipe400_, O_NONBLOCK);
  pipe2(pipe404_, O_NONBLOCK);
  for (size_t i = 0; i < THREADS_COUNT; ++i) {
    pipe2(outPipes_[i], O_NONBLOCK);
  }

  {
    write(pipe200_[1], RESPONSE_200, RESPONSE_200_SIZE);
    /*struct iovec iov{RESPONSE_200, RESPONSE_200_SIZE};
    ssize_t res = vmsplice(pipe200_[1], &iov, 1, 0);
    if (-1 == res) {
      return Result::FAILED;
    }*/
  }
  {
    write(pipe400_[1], RESPONSE_400, RESPONSE_400_SIZE);
    /*struct iovec iov{RESPONSE_400, RESPONSE_400_SIZE};
    ssize_t res = vmsplice(pipe400_[0], &iov, 1, 0);
    if (-1 == res) {
      return Result::FAILED;
    }*/
  }
  {
    write(pipe404_[1], RESPONSE_404, RESPONSE_404_SIZE);
    /*struct iovec iov{RESPONSE_404, RESPONSE_404_SIZE};
    ssize_t res = vmsplice(pipe404_[0], &iov, 1, 0);
    if (-1 == res) {
      return Result::FAILED;
    }*/
  }

  return Result::SUCCESS;
#endif
}

void HttpServer::acceptSocket(tcp::SocketWrapper sock)
{
  int flags = fcntl(int(sock), F_GETFL, 0);
  if (-1 == flags) {
    LOG_CRITICAL(stderr, "Cannot get fcntl flags, errno = %s(%d)\n", std::strerror(errno), errno);
    return ;
  }
  int res = fcntl(int(sock), F_SETFL, flags | O_NONBLOCK);
  if (-1 == res) {
    LOG_CRITICAL(stderr, "Cannot set fcntl O_NONBLOCK flag, errno = %s(%d)\n", std::strerror(errno), errno);
    return ;
  }

  threadPool_.run(sock);
}

} // namespace http
