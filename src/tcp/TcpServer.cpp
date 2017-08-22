#include <cstring>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "TcpServer.h"

namespace tcp
{

TcpServer::TcpServer()
{}

TcpServer::~TcpServer()
{
  stop();
}

void TcpServer::acceptFunc()
{
  SocketWrapper clientSock;
  int rawSock;
  do {
    clientSock = sock_.accept();
    rawSock = int(clientSock);
    acceptSocket(clientSock);
  } while (rawSock >= 0);
}

Result TcpServer::start(const uint16_t port)
{
  int res = sock_.create();
  if (res < 0) {
    LOG(stderr, "Cannot create server socket. errno = %d(%s)\n",
      errno, std::strerror(errno));
    return Result::FAILED;
  }
  {
    int one = 1;
    int res = sock_.setsockopt(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set SO_REUSEADDR | SO_REUSEPORT on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }
  res = sock_.bind(port);
  if (res < 0)
  {
    LOG(stderr, "Cannot bind server socket %d. errno = %d(%s)\n",
      int(sock_), errno, std::strerror(errno));
    return Result::FAILED;
  }

  res = sock_.listen();
  if (res < 0)
  {
    LOG(stderr, "Cannot bind server socket. errno = %d(%s)\n",
      errno, std::strerror(errno));
    return Result::FAILED;
  }
  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_DEFER_ACCEPT, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_DEFER_ACCEPT on socket, errno = %s(%d)\n", std::strerror(errno), errno);
    }
  }

  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_NODELAY on socket, errno = %s(%d)\n", std::strerror(errno), errno);
    }
  }
  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_QUICKACK on socket, errno = %s(%d)\n", std::strerror(errno), errno);
    }
  }

  {
    int bufferSize = 4 * 1024;
    int res = sock_.setsockopt(SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set SO_SNDBUF on socket, errno = %s(%d)\n", std::strerror(errno), errno);
    }
    res = sock_.setsockopt(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set SO_RCVBUF on socket, errno = %s(%d)\n", std::strerror(errno), errno);
    }
  }

  std::thread tmpThread(&TcpServer::acceptFunc, this);
  acceptThread_ = std::move(tmpThread);

  return Result::SUCCESS;
}

Result TcpServer::stop()
{
  if (acceptThread_.joinable()) {
    sock_.shutdown();
    acceptThread_.join();
    acceptThread_ = std::thread();
    sock_.close();
  }
  return Result::SUCCESS;
}

} // namespace tcp