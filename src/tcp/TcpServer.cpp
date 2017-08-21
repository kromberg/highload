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
  Socket clientSock;
  int rawSock;
  do {
    clientSock = sock_.accept();
    rawSock = int(clientSock);
    acceptSocket(std::move(clientSock));
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
  res = sock_.bind(port);
  if (res < 0)
  {
    LOG(stderr, "Cannot bind server socket. errno = %d(%s)\n",
      errno, std::strerror(errno));
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
    sock_.setsockopt(IPPROTO_TCP, TCP_DEFER_ACCEPT, &one, sizeof(one));
  }
  {
    int one = 1;
    sock_.setsockopt(IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
  }
  {
    int one = 1;
    sock_.setsockopt(IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));
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
  }
  return Result::SUCCESS;
}

} // namespace tcp