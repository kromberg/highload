#include <atomic>

#include <common/Types.h>

#include "ThreadPool.h"

namespace http
{

ThreadPool::ThreadPool(const size_t threadsCount, const Callback& cb):
  threadsCount_(threadsCount),
  currentThreadId_(0)
{
  threads_.reserve(threadsCount_);
  threadsInfo_.reserve(threadsCount_);
  for (size_t i = 0; i < threadsCount_; ++i) {
    threadsInfo_.emplace_back(new ThreadInfo(cb));
    threads_.emplace_back(&ThreadPool::threadFunc, threadsInfo_[i]);
  }
}

ThreadPool::~ThreadPool()
{
  for (auto ti : threadsInfo_) {
    ti->running = false;
    //ti->queue.abort();
  }
  for (auto& t : threads_) {
    t.join();
  }
  for (auto ti : threadsInfo_) {
    delete ti;
  }
}

void ThreadPool::threadFunc(ThreadInfo* ti)
{
  tcp::SocketWrapper sock;
  while (ti->running) {
    if (!ti->queue.try_pop(sock)) {
      continue;
    }
    ti->cb(sock);
  }
}

void ThreadPool::run(tcp::SocketWrapper& sock)
{
  size_t threadId = currentThreadId_.fetch_add(1);

  try {
    threadsInfo_[threadId % threadsCount_]->queue.push(sock);
  } catch (...) {
    LOG_CRITICAL(stderr, "cannot start new thread %zu\n", threadId);
  }
}

} // namespace http
