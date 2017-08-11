#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Socket.h"

namespace tcp
{
Socket::Socket(int sock):
  sock_(sock)
{}

Socket::~Socket()
{
  shutdown();
  close();
}

int Socket::create()
{
  if (sock_) {
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
} // namespace tcp