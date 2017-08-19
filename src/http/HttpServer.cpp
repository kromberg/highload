#include <thread>
#include <sstream>
#include <cstring>
#include <unordered_map>

#include "StateMachine.h"
#include "HttpServer.h"

namespace http
{

#define RESPONSE_HEADER_FORMAT \
  "HTTP/1.1 %d %s\n"\
  "Content-Type: application/json; charset=UTF-8\n"\
  "Connection: close\n"\
  "Content-Length: %zu\n"\
  "\n"\

#define RESPONSE_HEADER_ESTIMATED_SIZE 94 + 3 + 64 + 4

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
    LOG(stderr, "next: %c\n", *next);
    *next = '\0';
    req.params_ = next + 1;
    req.paramsSize_ = urlSize - (req.params_ - url);
    urlSize -= (req.paramsSize_ + 1); 
    LOG(stderr, "PARAMS: %s\n", req.params_);
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
        LOG(stderr, "Searching for table: %s\n", prev);
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
        LOG(stderr, "Error state\n");
        return HTTPCode::BAD_REQ;
    }
    LOG(stderr, "New state = %d\n", state);

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
  //LOG(stderr, "Request method: %s\n", reqMethod);
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
  } else if (size == 1 && *header == '\r') {
    hasNext = false;
    return HTTPCode::OK;
  }
  //LOG(stderr, "HEADER: %s\n", header);
  char* val = strchr(header, ':');
  if (!val || (val - header) > size) {
    return HTTPCode::BAD_REQ;
  }
  *val = '\0';
  if (0 == strncmp(header, "Content-Length", val - header)) {
    char* end;
    req.contentLength_ = static_cast<int32_t>(strtol(val + 1, &end, 10));
    if (!end) {
      return HTTPCode::NOT_FOUND;
    }
  } else if (0 == strncmp(header, "Content-Type", val - header)) {
    req.hasContentType_ = true;
  }
  hasNext = true;
  return HTTPCode::OK;
}

void HttpServer::handleRequest(tcp::Socket&& sock)
{
  std::string body("{}");
  Request req;
  HTTPCode code = readRequest(req, sock);
  if (HTTPCode::OK != code) {
    sendResponse(sock, code, body);
    return ;
  }

  StateMachine::Handler handler = StateMachine::getHandler(req);
  if (!handler) {
    LOG(stderr, "Cannot find handler");
    sendResponse(sock, code, body);
    return ;
  }

  code = handler(body, *storage_, req);
  if (HTTPCode::OK != code) {
    sendResponse(sock, code, body);
    return ;
  }

  sendResponse(sock, code, body);
}

HTTPCode HttpServer::readRequest(Request& req, tcp::Socket& sock)
{
  enum class State
  {
    METHOD,
    HEADERS,
    BODY,
    END,
  } state = State::METHOD;
  HTTPCode code;
  char buffer[8 * 1024];
  int offset = 0, size = 0;
  int res;
  do {
    res = sock.recv(buffer + size, sizeof(buffer) - size);
    if (res) {
      size += res;
      buffer[size] = '\0';
      if (State::BODY == state) {
        if (Type::GET == req.type_) {
          return HTTPCode::OK;
        }
        if (size - offset < req.contentLength_) {
          continue;
        }
        if (req.json_.Parse(buffer + offset).HasParseError()) {
          return HTTPCode::BAD_REQ;
        }
        return HTTPCode::OK;
      }
      char* next = strchr(buffer + offset, '\n');
      while (next && offset < size) {
        *next = '\0';
        switch (state) {
          case State::METHOD:
          {
            LOG(stderr, "Method : %s\n\n", buffer + offset);
            code = parseRequestMethod(req, buffer + offset, next - buffer - offset);
            if (HTTPCode::OK != code) {
              LOG(stderr, "Method : %s. Code : %d\n\n", buffer + offset, code);
              return code;
            }
            state = State::HEADERS;
            break;
          }
          case State::HEADERS:
          {
            LOG(stderr, "Header : %s\n\n", buffer + offset);
            bool hasNext;
            code = parseHeader(req, hasNext, buffer + offset, next - buffer - offset);
            if (HTTPCode::OK != code) {
              LOG(stderr, "Header : %s. Code : %d\n\n", buffer + offset, code);
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
          if (Type::GET == req.type_) {
            return HTTPCode::OK;
          }
          if (size - offset < req.contentLength_) {
            break;
          }
          state = State::END;
          if (req.json_.Parse(buffer + offset).HasParseError()) {
            return HTTPCode::BAD_REQ;
          }
          return HTTPCode::OK;
        }
        next = strchr(buffer + offset, '\n');
      }
    }
  } while (res >= 0);
  return code;
}

void HttpServer::sendResponse(tcp::Socket& sock, const HTTPCode code, const std::string& body)
{
  size_t estimatedSize = RESPONSE_HEADER_ESTIMATED_SIZE + body.size() + 1;
  char* response = new char[estimatedSize];
  int size = snprintf(response, estimatedSize, RESPONSE_HEADER_FORMAT "%s",
    code, httpCodeToStr(code), body.size(), body.c_str());
  response[size] = '\0';
  ++ size;

  //LOG(stderr, "Sending response: %s\n", response);

  send(sock, response, size);
  delete[] response;
}

void HttpServer::send(tcp::Socket& sock, const char* buffer, int size)
{
  if (size <= 0) {
    return ;
  }

  int offset = 0;
  while (offset < size) {
    int sent = sock.send(buffer + offset, size - offset);
    if (sent < 0) {
      break ;
    }
    offset += sent;
  }
}

void HttpServer::acceptSocket(tcp::Socket&& sock)
{
  std::thread{&HttpServer::handleRequest, this, std::move(sock)}.detach();
}

} // namespace http
