#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>

#include <tbb/concurrent_queue.h>

#include <tcp/Socket.h>

namespace http
{

class ThreadPool
{
public:
  typedef std::function<void(tcp::SocketWrapper& sock)> Callback;
private:
  struct ThreadInfo
  {
    Callback cb;
    tbb::concurrent_bounded_queue<tcp::SocketWrapper> queue;

    volatile bool running = true;

    ThreadInfo(const Callback& _cb) :
      cb(_cb)
    {
      queue.set_capacity(100);
    }
  };

  const size_t threadsCount_;
  std::atomic<size_t> currentThreadId_;
  std::vector<std::thread> threads_;
  std::vector<ThreadInfo*> threadsInfo_;

private:
  static void threadFunc(ThreadInfo* ti);

public:
  ThreadPool(const size_t threadsCount, const Callback& cb);
  ~ThreadPool();

  void run(tcp::SocketWrapper& sock);
};

} // namespace http

#endif // _THREAD_POOL_H_