#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

#include <tbb/concurrent_queue.h>

#include <tcp/Socket.h>

namespace http
{

class ThreadPool
{
public:
  typedef std::function<void(tcp::Socket&& sock)> Callback;
private:
  struct ThreadInfo
  {
    const int32_t id;
    Callback cb;
    tbb::concurrent_bounded_queue<size_t>& queue;

    tcp::Socket sock{-1};
    std::mutex mut;
    std::condition_variable cv;

    volatile bool working = false;
    volatile bool running = true;

    ThreadInfo(
      const int32_t _id,
      const Callback& _cb,
      tbb::concurrent_bounded_queue<size_t>& _queue) :
      id(_id), cb(_cb), queue(_queue)
    {}
  };

  const size_t threadsCount_;
  std::vector<std::thread> threads_;
  std::vector<ThreadInfo*> threadsInfo_;
  tbb::concurrent_bounded_queue<size_t> emptyThreads_;

private:
  static void threadFunc(ThreadInfo* ti);

public:
  ThreadPool(const size_t threadsCount, const Callback& cb);
  ~ThreadPool();

  void run(tcp::Socket&& sock);
};

} // namespace http

#endif // _THREAD_POOL_H_