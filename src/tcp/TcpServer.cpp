#include <cstring>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>

#include <common/Profiler.h>

#include "TcpServer.h"

namespace tcp
{

TcpServer::TcpServer(const size_t acceptThreadsCount):
  acceptThreads_(acceptThreadsCount)
{}

TcpServer::~TcpServer()
{
  stop();
}

Result TcpServer::start(const uint16_t port)
{
  for (auto& t : acceptThreads_) {
    t = std::make_shared<AcceptingThread>(std::bind(&TcpServer::acceptSocket, this, std::placeholders::_1));
    Result res = t->start();
    if (Result::SUCCESS != res) {
      return res;
    }
  }

  return doStart();
}

Result TcpServer::stop()
{
  for (auto& t : acceptThreads_) {
    t->stop();
  }

  return doStop();
}

int TcpServer::epoll_create(int size)
{
  START_PROFILER("epoll_create");
  return ::epoll_create(size);
}

int TcpServer::epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  START_PROFILER("epoll_wait");
  return ::epoll_wait(epfd, events, maxevents, timeout);
}

int TcpServer::epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask)
{
  START_PROFILER("epoll_pwait");
  return ::epoll_pwait(epfd, events, maxevents, timeout, sigmask);
}

int TcpServer::epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  START_PROFILER("epoll_ctl");
  return ::epoll_ctl(epfd, op, fd, event);
}

} // namespace tcp