#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <atomic>

#include <db/Storage.h>
#include <tcp/TcpServer.h>
#include <common/Types.h>

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

  struct Context
  {
    int fd;
    bool keepalive;
    Response resp;
  };

  static constexpr int MAX_EVENTS = 512;
  static constexpr size_t CONTEXT_POOL_SIZE = 16 * MAX_EVENTS;
  size_t currCtxIdx_ = 0;
  Context ctx_[CONTEXT_POOL_SIZE];

  Context listenCtx_;

private:
  void eventsThreadFunc();
  bool handleRequest(Context& ctx);
  bool handleRequestResponse(Context& ctx);

  HTTPCode parseURL(Request& req, char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, bool& hasNext, char* header, int32_t size);
  HTTPCode readRequest(Request& req, tcp::SocketWrapper sock);

  void setResponse(Response& resp, const HTTPCode code, const bool keepalive);

  void sendResponse(Context& ctx);
  void sendResponse(tcp::SocketWrapper sock, const HTTPCode code, bool keepalive);
  void sendResponse(tcp::SocketWrapper sock, const Response& resp);
  
  void send(tcp::SocketWrapper sock, const char* buffer, int32_t size);
  void write(int fd, const char* buffer, int32_t size);

  virtual Result doStart() override;
  virtual Result doStop() override;
public:
  HttpServer(db::StoragePtr& storage);
  ~HttpServer();
};
} // namespace http

#endif // _HTTP_SERVER_H