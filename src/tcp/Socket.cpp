#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <common/Profiler.h>

#include "Socket.h"

namespace tcp
{
using common::TimeProfiler;

Socket::Socket(int sock):
  sock_(sock)
{}

Socket::~Socket()
{
  shutdown();
  close();
}

Socket::Socket(Socket&& rhs):
  sock_(rhs.sock_)
{
  rhs.sock_ = -1;
}

Socket& Socket::operator=(Socket&& rhs)
{
  sock_ = rhs.sock_;
  rhs.sock_ = -1;

  return *this;
}

int Socket::create()
{
  if (sock_ >= 0) {
    return sock_;
  }
  int tmp_sock = ::socket(AF_INET, SOCK_STREAM, 0);
  sock_ = tmp_sock;
  return sock_;
}

int Socket::bind(const uint16_t port)
{
  if (sock_ < 0) {
    return -1;
  }
  struct sockaddr_in addr = {0};
    
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return ::bind(sock_, (struct sockaddr *)&addr, sizeof(addr));
}

int Socket::listen(int backlog)
{
  return ::listen(sock_, backlog);
}

Socket Socket::accept()
{
  struct sockaddr_in addr = {0};
  socklen_t addrLen = sizeof(addr);
  return ::accept(sock_, (struct sockaddr *) &addr, &addrLen);
}

int Socket::shutdown(int how)
{
  return ::shutdown(sock_, how);
}

int Socket::close()
{
  int res = ::close(sock_);
  sock_ = -1;
  return res;
}

int Socket::send(const char* buffer, size_t size, int flags)
{
  //TimeProfiler tp("send");
  return ::send(sock_, buffer, size, flags);
}

int Socket::recv(char* buffer, size_t size, int flags)
{
  //TimeProfiler tp("recv");
  return ::recv(sock_, buffer, size, flags);
}

int Socket::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
{
  return ::setsockopt(sock_, level, optname, optval, optlen);
}
} // namespace tcp