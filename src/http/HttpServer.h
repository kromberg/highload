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
  static constexpr size_t THREADS_COUNT = 4;
  db::StoragePtr storage_;

  std::thread eventsThread_;
  volatile bool running_ = true;
  int epollFd_;

  static constexpr size_t RESPONSES_POOL_SIZE = 256;
  uint8_t currentResponseIdx_ = 0;
  Response responses_[RESPONSES_POOL_SIZE];

  //ThreadPool threadPool_;

#if 0
  int pipe200_[2];
  int pipe400_[2];
  int pipe404_[2];
  int outPipes_[THREADS_COUNT][2];
  //std::atomic<size_t> threadIdx_;
#endif

private:
  void eventsThreadFunc();
  bool handleRequest(struct epoll_event& ev);
  virtual void acceptSocket(tcp::SocketWrapper sock) override;

  HTTPCode parseURL(Request& req, char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, bool& hasNext, char* header, int32_t size);
  HTTPCode readRequest(Request& req, tcp::SocketWrapper sock);

  void setResponse(struct epoll_event& ev, Response& resp, const HTTPCode code);
  void setResponse(struct epoll_event& ev, Response& resp);

  void sendResponse(struct epoll_event& ev);
  void sendResponse(struct epoll_event& ev, const Response& resp);
  void sendResponse(tcp::SocketWrapper sock, const HTTPCode code, bool keepalive);
  
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