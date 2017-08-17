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

HTTPCode HttpServer::parseURL(Request& req, const char* url, int32_t urlSize)
{
  static std::unordered_map<std::string, Table> strToTableMap = 
  {
    { "users",     Table::USERS },
    { "locations", Table::LOCATIONS },
    { "visits",    Table::VISITS },
    { "avg",       Table::AVG },
  };

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

  const char* next = strchr(url + 1, '?');
  if (next) {
    req.params_ = next + 1;
    req.paramsSize_ = urlSize - (next - url);
  }

  const char* prev = nullptr;
  next = url;
  do
  {
    prev = next + 1;
    next = strchr(prev, '/');
    int size;
    if (!next) {
      size = urlSize - (prev - url);
    } else {
      size = next - prev;
    }

    switch (state) {
      case State::TABLE1:
      case State::TABLE2:
      {
        auto it = strToTableMap.find(std::string(prev, size));
        if (strToTableMap.end() == it) {
          return HTTPCode::BAD_REQ;
        }
        if (State::TABLE1 == state) {
          req.table1_ = it->second;
          state = State::ID;
        } else {
          req.table2_ = it->second;
          state = State::ERROR;
        }
        break;
      }
      case State::ID:
      {
        if (0 == strncmp(prev, "new", size)) {
          req.id_ = -1;
        } else {
          try {
            req.id_ = std::stoul(std::string(prev, size));
          } catch (std::invalid_argument &e) {
            return HTTPCode::NOT_FOUND;
          }
        }
        break;
      }
      case State::ERROR:
        return HTTPCode::BAD_REQ;
    }

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
    return code;
  }

  return HTTPCode::OK;
}

HTTPCode HttpServer::parseHeader(Request& req, const char* header, int32_t size)
{
  const char* val = strchr(header, ':');
  if (!val || (val - header) > size) {
    return HTTPCode::BAD_REQ;
  }
  if (0 == strncmp(header, "Content-Length", val - header)) {
    try {
      req.contentLength_ = std::stoi(std::string(header, val - header));
    } catch (std::invalid_argument& e) {
      return HTTPCode::BAD_REQ;
    }
  } else if (0 == strncmp(header, "Content-Type", val - header)) {
    req.hasContentType_ = true;
  }
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
        if (size - offset < req.contentLength_) {
          continue;
        }
        if (Type::GET == req.type_) {
          break;
        }
        if (req.json_.Parse(buffer + offset).HasParseError()) {
          return HTTPCode::BAD_REQ;
        }
        state = State::END;
        break;
      }
      char* next = strchr(buffer + offset, '\n');
      while (next) {
        *next = '\0';
        switch (state) {
          case State::METHOD:
          {
            code = parseRequestMethod(req, buffer + offset, next - buffer - offset);
            if (HTTPCode::OK != code) {
              return code;
            }
            state = State::HEADERS;
            break;
          }
          case State::HEADERS:
          {
            if (next == buffer + offset + 1) {
              state = State::BODY;
              break;
            }
            code = parseHeader(req, buffer + offset, next - buffer - offset);
            if (HTTPCode::OK != code) {
              return code;
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
          state = State::END;
          if (Type::GET == req.type_) {
            break;
          }
          if (req.json_.Parse(buffer + offset).HasParseError()) {
            return HTTPCode::BAD_REQ;
          }
        }
        next = strchr(buffer + offset, '\n');
      }
    }
  } while (res >= 0 && State::END != state);
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
