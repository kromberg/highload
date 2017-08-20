#include <common/Types.h>

#include "ThreadPool.h"

namespace http
{

ThreadPool::ThreadPool(const size_t threadsCount, const Callback& cb):
  threadsCount_(threadsCount)
{
  threads_.reserve(threadsCount_);
  threadsInfo_.reserve(threadsCount_);
  emptyThreads_.set_capacity(threadsCount_);
  for (size_t i = 0; i < threadsCount_; ++i) {
    threadsInfo_.emplace_back(new ThreadInfo(i, cb, emptyThreads_));
    threads_.emplace_back(&ThreadPool::threadFunc, threadsInfo_[i]);
    emptyThreads_.push(i);
  }
}

ThreadPool::~ThreadPool()
{
  for (auto ti : threadsInfo_) {
    ti->running = false;
    ti->working = true;
    ti->cv.notify_one();
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
  while (ti->running) {
    std::unique_lock<std::mutex> lock(ti->mut);
    while (!ti->working) {
      ti->cv.wait(lock);
    }

    if (-1 == ti->sock) {
      continue;
    }

    ti->cb(std::move(ti->sock));

    ti->working = false;
    ti->queue.push(ti->id);
  }
}

void ThreadPool::run(tcp::Socket&& sock)
{
  size_t threadId;
  emptyThreads_.pop(threadId);

  threadsInfo_[threadId]->sock = std::move(sock);
  threadsInfo_[threadId]->working = true;
  threadsInfo_[threadId]->cv.notify_one();
  LOG(stderr, "running new thread: %zu\n", threadId);
}

} // namespace http
