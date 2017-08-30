#include <thread>
#include <sstream>
#include <cstring>
#include <unordered_map>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>

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
  storage_(storage)
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
    req.params = next + 1;
    req.paramsSize = urlSize - (req.params - url);
    urlSize -= (req.paramsSize + 1); 
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
          req.table1 = it->second;
          state = State::ID;
        } else {
          LOG(stderr, "TABLE 2: %s\n", prev);
          req.table2 = it->second;
          state = State::ERROR;
        }
        break;
      }
      case State::ID:
      {
        LOG(stderr, "ID: %s\n", prev);
        if (0 == strncmp(prev, "new", size)) {
          req.id = -1;
        } else {
          char* end;
          req.id = static_cast<int32_t>(strtol(prev, &end, 10));
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
    req.type = Type::POST;
  } else if (0 == strncmp(reqMethod, "GET", next - reqMethod)) {
    req.type = Type::GET;
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
      req.contentLength = static_cast<int32_t>(strtol(val + 1, &end, 10));
      if (!end) {
        return HTTPCode::NOT_FOUND;
      }
    } else if (0 == strncmp(header + 7, "-Type", 5)) {
      req.hasContentType = true;
    }
  }
  hasNext = true;
  return HTTPCode::OK;
}

Result HttpServer::handleRequest(tcp::SocketWrapper sock)
{
  Request req;
  std::pair<Result, HTTPCode> res = readRequest(req, sock);
  if (Result::SUCCESS == res.first ||
      Result::FAILED  == res.first) {
    return res.first;
  }
  HTTPCode code = res.second;
  if (HTTPCode::OK != code) {
    sendResponse(sock, code, req.keepalive);
    return res.first;
  }

  StateMachine::Handler handler = StateMachine::getHandler(req);
  if (!handler) {
    sendResponse(sock, code, req.keepalive);
    return res.first;
  }

  Response resp;
  {
    //START_PROFILER("handler");
    code = handler(resp, *storage_, req);
  }
  if (HTTPCode::OK != code) {
    sendResponse(sock, code, req.keepalive);
    return res.first;
  }

  if (Type::POST == req.type) {
    sendResponse(sock, HTTPCode::OK, false);
  } else {
    sendResponse(sock, resp);
  }

  return res.first;
}

std::pair<Result, HTTPCode> HttpServer::readRequest(Request& req, tcp::SocketWrapper sock)
{
  enum class State
  {
    METHOD,
    HEADERS,
    BODY,
  } state = State::METHOD;
  HTTPCode code = HTTPCode::OK;
  char buffer[BUFFER_SIZE];
  int offset = 0, size = 0;
  int res;
  for (;;) {
    res = sock.recv(buffer + size, sizeof(buffer) - size, MSG_DONTWAIT);
    if (0 == res) {
      return std::make_pair(Result::FAILED, HTTPCode::OK);
    } else if (res < 0) {
      if (EINTR == errno) {
        continue;
      } else if (EWOULDBLOCK == errno || EAGAIN == errno) {
        return std::make_pair(Result::SUCCESS, HTTPCode::OK);
      }
      return std::make_pair(Result::FAILED, HTTPCode::OK);
    } else {
      size += res;
      buffer[size] = '\0';
      LOG(stderr, "Received buffer %s\n", buffer);
      if (State::BODY == state) {
        if (size - offset < req.contentLength) {
          continue;
        }
        req.content = buffer + offset;
        return std::make_pair(Result::CLOSE, HTTPCode::OK);
      }
      char* next = strchr(buffer + offset, '\n');
      while (next && offset < size) {
        *next = '\0';
        switch (state) {
          case State::METHOD:
          {
            //START_PROFILER("parseRequestMethod");
            code = parseRequestMethod(req, buffer + offset, next - buffer - offset);
            //STOP_PROFILER;
            if (Type::GET == req.type) {
              req.keepalive = true;
              return std::make_pair(Result::AGAIN, code);
            }
            if (HTTPCode::OK != code) {
              LOG(stderr, "Method : %s. Code : %d\n", buffer + offset, code);
              return std::make_pair(Result::CLOSE, code);
            }
            state = State::HEADERS;
            break;
          }
          case State::HEADERS:
          {
            //START_PROFILER("parseHeader");
            bool hasNext;
            code = parseHeader(req, hasNext, buffer + offset, next - buffer - offset);
            //STOP_PROFILER;
            if (HTTPCode::OK != code) {
              LOG(stderr, "Header : %s. Code : %d\n", buffer + offset, code);
              return std::make_pair(Result::CLOSE, code);
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
          if (size - offset < req.contentLength) {
            break;
          }
          req.content = buffer + offset;
          return std::make_pair(Result::CLOSE, HTTPCode::OK);
        }
        next = strchr(buffer + offset, '\n');
      }
    }
  }
  return std::make_pair(Result::CLOSE, code);
}

void HttpServer::sendResponse(tcp::SocketWrapper sock, const HTTPCode code, bool keepalive)
{
  size_t idx = keepalive ? 1 : 0;
  switch (code) {
    case HTTPCode::OK:
    {
      send(sock, RESPONSE_200[idx], RESPONSE_200_SIZE[idx]);
      break;
    }
    case HTTPCode::BAD_REQ:
    {
      send(sock, RESPONSE_400[idx], RESPONSE_400_SIZE[idx]);
      break;
    }
    case HTTPCode::NOT_FOUND:
    {
      send(sock, RESPONSE_404[idx], RESPONSE_404_SIZE[idx]);
      break;
    }
    default:
      break;
  }
}

void HttpServer::sendResponse(tcp::SocketWrapper sock, const Response& resp)
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

void HttpServer::eventsThreadFunc()
{
  struct epoll_event ev, events[MAX_EVENTS];
  int nfds;

  do {
    nfds = epoll_wait(epollFd_, events, MAX_EVENTS, -1);
    int i;
    for (i = 0; i < nfds; ++i) {
      if (events[i].data.fd == int(sock_)) {
        for (;;) {
          tcp::SocketWrapper sock = sock_.accept4(SOCK_NONBLOCK);
          if (-1 == int(sock)) {
            break;
          }

          {
            int one = 1;
            int res = sock.setsockopt(IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            if (0 != res) {
              LOG_CRITICAL(stderr, "Cannot set TCP_NODELAY on socket, errno = %s(%d)\n", std::strerror(errno), errno);
              continue;
            }
          }

          {
            int one = 1;
            int res = sock.setsockopt(IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));
            if (0 != res) {
              LOG_CRITICAL(stderr, "Cannot set TCP_QUICKACK on socket, errno = %s(%d)\n", std::strerror(errno), errno);
              continue;
            }
          }

          bool closed = false;
          for(;;) {
            Result res = handleRequest(sock);
            if (Result::CLOSE == res ||
                Result::FAILED == res)
            {
              sock.shutdown();
              sock.close();
              closed = true;
              break;
            } else if (Result::SUCCESS == res) {
              break;
            }
          } // for(;;)

          if (closed) {
            continue;
          }

          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = int(sock);
          if (-1 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, int(sock), &ev)) {
            LOG_CRITICAL(stderr, "epoll_ctl error, errno = %s(%d)\n", std::strerror(errno), errno);
            continue;
          }
        } // for (;;)
      } else {
        tcp::SocketWrapper sock(events[i].data.fd);
        if ((events[i].events & EPOLLERR)   ||
            (events[i].events & EPOLLHUP)   ||
           !(events[i].events & EPOLLIN))
        {
          sock.shutdown();
          sock.close();
          continue;
        }
        for(;;) {
          Result res = handleRequest(sock);
          if (Result::CLOSE == res ||
              Result::FAILED == res)
          {
            sock.shutdown();
            sock.close();
            break;
          } else if (Result::SUCCESS == res) {
            break;
          }
        } // for(;;)
      }
    } // for (i = 0; i < nfds; ++i) {

  } while (running_ && nfds >= 0);
  LOG_CRITICAL(stderr, "Events thread is stopped\n");
}

Result HttpServer::doStart()
{
  epollFd_ = epoll_create(64 * 1024);
  if (-1 == epollFd_) {
    return Result::FAILED;
  }
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = int(sock_);
  if (-1 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, int(sock_), &ev)) {
    return Result::FAILED;
  }

  std::thread tmpThread(&HttpServer::eventsThreadFunc, this);
  eventsThread_ = std::move(tmpThread);

  return Result::SUCCESS;
}

Result HttpServer::doStop()
{
  if (eventsThread_.joinable()) {
    running_ = false;
    sock_.close();
    close(epollFd_);
    eventsThread_.join();
    eventsThread_ = std::thread();
  }
  return Result::SUCCESS;
}

} // namespace http
