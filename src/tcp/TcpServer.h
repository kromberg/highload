#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <cstdint>
#include <thread>

#include <common/Types.h>

#include "Socket.h"

namespace tcp
{
using common::Result;

class TcpServer
{
private:
  Socket sock_;
  std::thread acceptThread_;

private:
  void acceptFunc();

public:
  TcpServer();
  virtual ~TcpServer();

  Result start(const uint16_t port = 80);
  Result stop();
};

} // namespace tcp
#endif // _TCP_SERVER_H_