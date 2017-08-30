#ifndef _ACCEPTING_THREAD_H_
#define _ACCEPTING_THREAD_H_

#include <cstdint>
#include <thread>
#include <functional>
#include <common/Types.h>

#include "Socket.h"

namespace tcp
{
using common::Result;

class AcceptingThread
{
protected:
  Socket sock_;
  std::thread thread_;
  typedef std::function<void(SocketWrapper sock)> Callback;
  Callback callback_;

private:
  void threadFunc();
public:
  AcceptingThread(const Callback& cb = Callback());
  virtual ~AcceptingThread();

  Result start(const uint16_t port = 80);
  void stop();
};

} // namespace tcp
#endif // _ACCEPTING_THREAD_H_