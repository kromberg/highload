#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <db/Storage.h>
#include <tcp/TcpServer.h>

#include "Types.h"

namespace http
{
class HttpServer : public tcp::TcpServer
{
private:
  db::StoragePtr storage_;
private:
  void handleRequest(tcp::Socket&& sock);
  virtual void acceptSocket(tcp::Socket&& sock) override;

  HTTPCode parseURL(Request& req, const std::string& url);
  HTTPCode parseRequestMethod(Request& req, const std::string& reqMethod);
  HTTPCode parseRequest(Request& req, std::stringstream& reqStream);

  void sendResponse(tcp::Socket& sock, const HTTPCode code, const std::string& body);
  void send(tcp::Socket& sock, const char* buffer, int size);

public:
  HttpServer(db::StoragePtr& storage);
  ~HttpServer();
};
} // namespace http

#endif // _HTTP_SERVER_H