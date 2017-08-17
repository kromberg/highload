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

  HTTPCode parseURL(Request& req, const char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, const char* header, int32_t size);
  HTTPCode readRequest(Request& req, tcp::Socket& sock);

  void sendResponse(tcp::Socket& sock, const HTTPCode code, const std::string& body);
  void send(tcp::Socket& sock, const char* buffer, int32_t size);

public:
  HttpServer(db::StoragePtr& storage);
  ~HttpServer();
};
} // namespace http

#endif // _HTTP_SERVER_H