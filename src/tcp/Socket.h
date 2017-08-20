#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>

namespace tcp
{
class Socket
{
private:
  int sock_;

public:
  Socket(int sock = -1);
  ~Socket();
  Socket(Socket&& rhs);
  Socket& operator=(Socket&& rhs);
  int create();
  int bind(const uint16_t port = 80);
  int listen(int backlog = 128);
  Socket accept();
  int shutdown(int how = SHUT_RDWR);
  int close();
  operator int() { return sock_; }
  int send(const char* buffer, size_t size, int flags = 0);
  int recv(char* buffer, size_t size, int flags = 0);

  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);

};
} // namespace tcp

#endif // _SOCKET_H_