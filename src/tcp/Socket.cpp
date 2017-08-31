#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <common/Profiler.h>

#include "Socket.h"

namespace tcp
{
Socket::Socket(int sock):
  SocketWrapper(sock)
{}

Socket::~Socket()
{
  //shutdown();
  close();
}

Socket::Socket(const SocketWrapper& rhs):
  SocketWrapper(rhs)
{}

SocketWrapper::SocketWrapper(int sock):
  sock_(sock)
{}

SocketWrapper::SocketWrapper(SocketWrapper&& rhs):
  sock_(rhs.sock_)
{
  rhs.sock_ = -1;
}

SocketWrapper& SocketWrapper::operator=(SocketWrapper&& rhs)
{
  sock_ = rhs.sock_;
  rhs.sock_ = -1;

  return *this;
}

int SocketWrapper::create()
{
  if (sock_ >= 0) {
    return sock_;
  }
  int tmp_sock = ::socket(AF_INET, SOCK_STREAM, 0);
  sock_ = tmp_sock;
  return sock_;
}

int SocketWrapper::bind(const uint16_t port)
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

int SocketWrapper::listen(int backlog)
{
  return ::listen(sock_, backlog);
}

SocketWrapper SocketWrapper::accept()
{
  START_PROFILER("accept");
  return ::accept(sock_, nullptr, nullptr);
}

SocketWrapper SocketWrapper::accept4(int flags)
{
  START_PROFILER("accept4");
  return ::accept4(sock_, nullptr, nullptr, flags);
}

int SocketWrapper::shutdown(int how)
{
  START_PROFILER("shutdown");
  return ::shutdown(sock_, how);
}

int SocketWrapper::close()
{
  START_PROFILER("close");
  int res = ::close(sock_);
  sock_ = -1;
  return res;
}

int SocketWrapper::send(const char* buffer, size_t size, int flags)
{
  START_PROFILER("send");
  return ::send(sock_, buffer, size, flags);
}

int SocketWrapper::recv(char* buffer, size_t size, int flags)
{
  START_PROFILER("recv");
  return ::recv(sock_, buffer, size, flags);
}

int SocketWrapper::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
{
  return ::setsockopt(sock_, level, optname, optval, optlen);
}

int SocketWrapper::getsockopt(int level, int optname, void *optval, socklen_t *optlen)
{
  return ::getsockopt(sock_, level, optname, optval, optlen);
}
} // namespace tcp