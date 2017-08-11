#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>

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
  int create();
  int bind(const uint16_t port = 80);
  int listen(int backlog = 128);
  Socket accept();
  int shutdown(int how = SHUT_RDWR);
  int close();
  operator int() { return sock_; }
};
} // namespace tcp

#endif // _SOCKET_H_