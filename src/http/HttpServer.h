#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <array>

#include <db/Storage.h>
#include <tcp/TcpServer.h>
#include <common/Types.h>
#include <common/Profiler.h>

#include "Types.h"
#include "ThreadPool.h"

namespace http
{

using common::Result;

class HttpServer : public tcp::TcpServer
{
private:
  db::StoragePtr storage_;

  std::thread eventsThread_;
  volatile bool running_ = true;
  int epollFd_;

  static constexpr size_t BUFFER_SIZE = 8 * 1024;
  static constexpr int MAX_EVENTS = 1024;
  Response resp_;
private:
  virtual void acceptSocket(tcp::SocketWrapper sock) override;

  void eventsThreadFunc();
  Result handleRequest(tcp::SocketWrapper sock);

  HTTPCode parseURL(Request& req, char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, bool& hasNext, char* header, int32_t size);
  std::pair<Result, HTTPCode> readRequest(Request& req, tcp::SocketWrapper sock);

  void sendResponse(tcp::SocketWrapper sock, const HTTPCode code, bool keepalive);
  void sendResponse(tcp::SocketWrapper sock, const Response& resp);
  
  void send(tcp::SocketWrapper sock, const char* buffer, int32_t size);
  void write(int fd, const char* buffer, int32_t size);

  virtual Result doStart() override;
  virtual Result doStop() override;
public:
  HttpServer(const size_t acceptThreadsCount, db::StoragePtr& storage);
  ~HttpServer();
};

} // namespace http

#endif // _HTTP_SERVER_H