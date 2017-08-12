#include <cstring>

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
  do {
    clientSock = sock_.accept();
    acceptSocket(std::move(clientSock));
  } while (int(clientSock) >= 0);
}

void TcpServer::acceptSocket(Socket&& sock)
{

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