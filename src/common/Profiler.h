#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <cstdint>
#include <time.h>
#include <unordered_map>

#include <tbb/spin_rw_mutex.h>

#include "Types.h"

namespace common
{

struct Operation
{
  uint64_t callsCount;
  uint64_t timeNsec;
};

class TimeProfiler
{
private:
  static tbb::spin_rw_mutex guard_;
  static std::unordered_map<std::string, Operation> operations_;

  std::string op_;
  bool running_ = true;
  struct timespec ts_;

public:
  template<class T>
  TimeProfiler(T&& op):
    op_(std::forward<T>(op))
  {
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_);
  }
  ~TimeProfiler()
  {
    if (running_) {
      stop();
    }
  }

  void stop()
  {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    tbb::spin_rw_mutex::scoped_lock l(guard_, true);
    Operation& operation = operations_[op_];
    ++ operation.callsCount;
    operation.timeNsec += ((end.tv_sec - ts_.tv_sec) * 1000000000UL + end.tv_nsec - ts_.tv_nsec);
    running_ = false;
  }

  static void print()
  {
    tbb::spin_rw_mutex::scoped_lock l(guard_, false);
    for (const auto& op : operations_) {
      LOG_CRITICAL(stderr, "%20s -> %20lu calls %20lu nsecs\n", op.first.c_str(), op.second.callsCount, op.second.timeNsec);
    }
  }
};

#define START_PROFILER(name) \
#ifdef PROFILER \
  common::TimeProfiler tp(name); \
#endif

#define STOP_PROFILER \
#ifdef PROFILER \
  tp.stop(); \
#endif


} // namespace common
#endif // _PROFILER_H_