#include <thread>
#include <sstream>
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

HTTPCode HttpServer::parseURL(Request& req, const std::string& url)
{
  static std::unordered_map<std::string, Table> strToTableMap = 
  {
    { "users",     Table::USERS },
    { "locations", Table::LOCATIONS },
    { "visits",    Table::VISITS },
    { "avg",       Table::AVG },
  };

  enum class State
  {
    TABLE1,
    ID,
    TABLE2,
  } state = State::TABLE1;

  size_t end = url.find('?', 1);
  if (std::string::npos != end) {
    req.params_ = url.substr(end + 1);
    LOG(stderr, "Requst params: %s\n", req.params_.c_str());
  }

  size_t pos1, pos2 = 0;
  do
  {
    pos1 = pos2 + 1;
    pos2 = url.find('/', pos1);
    if (std::string::npos == pos2) {
      pos2 = end;
    }
    size_t size = (std::string::npos == pos2) ? std::string::npos : (pos2 - pos1);

    switch (state) {
      case State::TABLE1:
      case State::TABLE2:
      {
        auto it = strToTableMap.find(url.substr(pos1, size));
        if (strToTableMap.end() == it) {
          return HTTPCode::BAD_REQ;
        }
        if (State::TABLE1 == state) {
          req.table1_ = it->second;
          state = State::ID;
        } else {
          req.table2_ = it->second;
        }
        break;
      }
      case State::ID:
      {
        std::string id = url.substr(pos1, size);
        if ("new" == id) {
          req.id_ = -1;
        } else {
          try {
            req.id_ = std::stoul(id);
          } catch (std::invalid_argument &e) {
            return HTTPCode::NOT_FOUND;
          }
        }
        break;
      }
    }

  } while (pos2 != end);

  return HTTPCode::OK;
}

HTTPCode HttpServer::parseRequestMethod(Request& req, const std::string& reqMethod)
{
  static std::string POST_STR = "POST";
  static std::string GET_STR = "GET";
  size_t pos1 = 0, pos2 = reqMethod.find(' ');
  std::string typeStr = reqMethod.substr(pos1, pos2);
  if (0 == reqMethod.compare(POST_STR)) {
    req.type_ = Type::POST;
  } else if (0 == reqMethod.compare(GET_STR)) {
    req.type_ = Type::GET;
  } else {
    req.type_ = Type::NONE;
  }
  pos1 = pos2 + 1;
  pos2 = reqMethod.find(' ', pos1);
  std::string url = reqMethod.substr(pos1, pos2 - pos1);

  HTTPCode code = parseURL(req, url);
  if (HTTPCode::OK != code) {
    return code;
  }

  return HTTPCode::OK;
}

HTTPCode HttpServer::parseRequest(Request& req, std::stringstream& reqStream)
{
  reqStream.seekp(0);

  std::string reqMethod;
  getline(reqStream, reqMethod);
  if (reqMethod.empty()) {
    return HTTPCode::BAD_REQ;
  }
  HTTPCode code = parseRequestMethod(req, reqMethod);
  if (HTTPCode::OK != code) {
    return code;
  }

  std::string header;
  int contentLength = 0;
  for (;;)
  {
    header.clear();
    getline(reqStream, header);
    if (header.empty()) {
      break;
    }
    size_t pos = header.find(':');
    std::string name = header.substr(0, pos);
    if ("Content-Length" == name) {
      try {
        contentLength = std::stoi(header.substr(pos + 1));
      } catch (std::invalid_argument& e) {
        return HTTPCode::BAD_REQ;
      }
    }
  }

  if (0 == contentLength) {
    return HTTPCode::OK;
  }

  std::string content;
  content.resize(contentLength);
  reqStream.get(&content[0], contentLength);
  req.json_.Parse(content.c_str());

  return HTTPCode::OK;
}

void HttpServer::handleRequest(tcp::Socket&& sock)
{
  std::stringstream reqStream;
  thread_local char buffer[128];
  int res = 0;
  do {
    int res = sock.recv(buffer, sizeof(buffer));
    if (res) {
      buffer[res] = 0;
      reqStream << buffer;
    }
  } while (res > 0);
  LOG(stderr, "Received request: %s\n", reqStream.str().c_str());

  std::string body("{}");
  Request req;
  HTTPCode code = parseRequest(req, reqStream);
  if (HTTPCode::OK != code) {
    sendResponse(sock, code, body);
    return ;
  }

  StateMachine::Handler handler = StateMachine::getHandler(req);
  if (!handler) {
    LOG(stderr, "There is no handler for requst\n");
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

void HttpServer::sendResponse(tcp::Socket& sock, const HTTPCode code, const std::string& body)
{
  char* response = new char[RESPONSE_HEADER_ESTIMATED_SIZE + body.size()];
  int size = snprintf(response, RESPONSE_HEADER_ESTIMATED_SIZE, RESPONSE_HEADER_FORMAT "%s",
    code, httpCodeToStr(code), body.size(), body.c_str());

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
