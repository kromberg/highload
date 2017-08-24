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
  static constexpr size_t THREADS_COUNT = 3;
  db::StoragePtr storage_;
  ThreadPool threadPool_;

  int pipe200_[2];
  int pipe400_[2];
  int pipe404_[2];
  int outPipes_[THREADS_COUNT][2];
  std::atomic<size_t> threadIdx_;
private:
  void handleRequest(tcp::SocketWrapper sock);
  virtual void acceptSocket(tcp::SocketWrapper sock) override;

  HTTPCode parseURL(Request& req, char* url, int32_t size);
  HTTPCode parseRequestMethod(Request& req, char* reqMethod, int32_t size);
  HTTPCode parseHeader(Request& req, bool& hasNext, char* header, int32_t size);
  HTTPCode readRequest(Request& req, tcp::SocketWrapper sock);

  void sendResponse(tcp::SocketWrapper sock, const HTTPCode code);
  void sendResponse(tcp::SocketWrapper sock, const std::string& body);
  void send(tcp::SocketWrapper sock, const char* buffer, int32_t size);
  void write(int fd, const char* buffer, int32_t size);

  virtual Result doStart() override;
public:
  HttpServer(db::StoragePtr& storage);
  ~HttpServer();
};
} // namespace http

#endif // _HTTP_SERVER_H