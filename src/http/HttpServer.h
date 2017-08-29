#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <array>

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

  static constexpr int MAX_EVENTS = 32;
  static constexpr size_t BUFFERS_POOL_SIZE = 1024;
  static constexpr size_t BUFFER_SIZE = 4 * 1024;
  static constexpr size_t FD_COUNT = 65536;
  size_t currentBufferIdx_ = 0;
  char bufferPool_[BUFFERS_POOL_SIZE][BUFFER_SIZE];
  enum class ConnectionStatus
  {
    OPEN,
    CLOSE_IMMEDIATE,
    CLOSE,
  };
  struct SendContext
  {
    bool keepalive;
    ConnectionStatus status;
    ConstBuffer buffer;
  };
  SendContext sendCtxes_[FD_COUNT];

private:
  template<size_t N>
  void addClosingFd(int fd, std::array<int, N>& fds, size_t& idx);
  template<size_t N>
  void closeFds(std::array<int, N> fds, const size_t size);

  void eventsThreadFunc();
  bool handleRequest(tcp::SocketWrapper sock);
  ConnectionStatus handleRequestResponse(tcp::SocketWrapper sock);

  HTTPCode parseURL(Request& req, char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, bool& hasNext, char* header, int32_t size);
  HTTPCode readRequest(Request& req, tcp::SocketWrapper sock);

  void setResponse(SendContext& sndCtx, const HTTPCode code);
  void setResponse(SendContext& sndCtx, const Response& resp);

  ConnectionStatus sendResponse(tcp::SocketWrapper sock);
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

template<size_t N>
void HttpServer::addClosingFd(int fd, std::array<int, N>& fds, size_t& idx)
{
  fds[idx ++] = fd;
  if (idx >= fds.size()) {
    std::thread{&HttpServer::closeFds<N>, this, fds, idx}.detach();
    idx = 0;
  }
}

template<size_t N>
void HttpServer::closeFds(std::array<int, N> fds, const size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    tcp::SocketWrapper sock(fds[i]);
    sock.shutdown();
    sock.close();
  }
}
} // namespace http

#endif // _HTTP_SERVER_H