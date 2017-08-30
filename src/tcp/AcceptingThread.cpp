#include <cstring>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>

#include <common/Profiler.h>

#include "AcceptingThread.h"

namespace tcp
{

AcceptingThread::AcceptingThread(const Callback& cb):
  callback_(cb)
{}

AcceptingThread::~AcceptingThread()
{
  stop();
}

Result AcceptingThread::start(const uint16_t port)
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

  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_NODELAY on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

  {
    int one = 1;
    int res = sock_.setsockopt(IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));
    if (0 != res) {
      LOG_CRITICAL(stderr, "Cannot set TCP_QUICKACK on socket, errno = %s(%d)\n", std::strerror(errno), errno);
      return Result::FAILED;
    }
  }

/*
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
*/

  res = sock_.bind(port);
  if (res < 0)
  {
    LOG_CRITICAL(stderr, "Cannot bind server socket %d. errno = %d(%s)\n",
      int(sock_), errno, std::strerror(errno));
    return Result::FAILED;
  }

  res = sock_.listen(64 * 1024);
  if (res < 0)
  {
    LOG_CRITICAL(stderr, "Cannot bind server socket. errno = %d(%s)\n",
      errno, std::strerror(errno));
    return Result::FAILED;
  }

  thread_ = std::thread{&AcceptingThread::threadFunc, this};

  return Result::SUCCESS;
}

void AcceptingThread::threadFunc()
{
  for (;;) {
    tcp::SocketWrapper sock = sock_.accept4(SOCK_NONBLOCK);
    if (-1 == int(sock)) {
      return;
    }
    callback_(sock);
  }
}

void AcceptingThread::stop()
{
  sock_.shutdown();
  sock_.close();
  if (thread_.joinable()) {
    thread_.join();
    thread_ = std::thread();
  }
}

} // namespace tcp