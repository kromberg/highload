#include <cstring>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include "TcpServer.h"

namespace tcp
{

TcpServer::TcpServer()
{}

TcpServer::~TcpServer()
{
  stop();
}

Result TcpServer::start(const uint16_t port)
{
  int res = sock_.create();
  if (res < 0) {
    LOG_CRITICAL(stderr, "Cannot create server socket. errno = %d(%s)\n",
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
    LOG_CRITICAL(stderr, "Cannot bind server socket %d. errno = %d(%s)\n",
      int(sock_), errno, std::strerror(errno));
    return Result::FAILED;
  }

  res = sock_.listen();
  if (res < 0)
  {
    LOG_CRITICAL(stderr, "Cannot bind server socket. errno = %d(%s)\n",
      errno, std::strerror(errno));
    return Result::FAILED;
  }
  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_DEFER_ACCEPT, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_DEFER_ACCEPT on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_NODELAY on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

  {
    int bufferSize = 8 * 1024;
    int res = sock_.setsockopt(SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set SO_SNDBUF on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
    res = sock_.setsockopt(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set SO_RCVBUF on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

  {
    int flags = fcntl(int(sock_), F_GETFL, 0);
    if (-1 == flags) {
      LOG_CRITICAL(stderr, "Cannot get socket flags, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }

    int res = fcntl(int(sock_), F_SETFL, flags | O_NONBLOCK);
    if (-1 == res) {
      LOG_CRITICAL(stderr, "Cannot set O_NONBLOCK on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

  return doStart();
}

Result TcpServer::stop()
{
  return doStop();
}

} // namespace tcp