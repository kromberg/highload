#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <cstdint>
#include <thread>
#include <vector>
#include <memory>

#include <sys/epoll.h>

#include <common/Types.h>

#include "Socket.h"
#include "AcceptingThread.h"

namespace tcp
{
using common::Result;

class TcpServer
{
protected:
  std::vector<std::shared_ptr<AcceptingThread> > acceptThreads_;

protected:
  int epoll_create(int size);
  int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
  int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask);
  int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

protected:
  virtual void acceptSocket(SocketWrapper sock) = 0;
  void acceptThreadFunc();
  virtual Result doStart() { return Result::SUCCESS; }
  virtual Result doStop() { return Result::SUCCESS; }
public:
  TcpServer(const size_t acceptThreadsCount);
  virtual ~TcpServer();

  Result start(const uint16_t port = 80);
  Result stop();
};

} // namespace tcp
#endif // _TCP_SERVER_H_