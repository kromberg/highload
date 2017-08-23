#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>

namespace tcp
{
class SocketWrapper
{
protected:
  int sock_;

public:
  SocketWrapper(int sock = -1);
  SocketWrapper(const SocketWrapper& rhs) = default;
  SocketWrapper& operator=(const SocketWrapper& rhs) = default;
  SocketWrapper(SocketWrapper&& rhs);
  SocketWrapper& operator=(SocketWrapper&& rhs);
  int create();
  int bind(const uint16_t port = 80);
  int listen(int backlog = 5);
  SocketWrapper accept();
  int shutdown(int how = SHUT_RDWR);
  int close();
  operator int() { return sock_; }
  int send(const char* buffer, size_t size, int flags = 0);
  int recv(char* buffer, size_t size, int flags = 0);

  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
  int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
};

class Socket : public SocketWrapper
{
public:
  Socket(int sock = -1);
  Socket(const SocketWrapper& rhs);
  ~Socket();
  Socket(const Socket& rhs) = delete;
  Socket& operator=(const Socket& rhs) = delete;
};
} // namespace tcp

#endif // _SOCKET_H_