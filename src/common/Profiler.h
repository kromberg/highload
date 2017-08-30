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
  static std::unordered_map<std::string, Operation> instantaneousOperations_;

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
    {
      Operation& operation = operations_[op_];
      ++ operation.callsCount;
      operation.timeNsec += ((end.tv_sec - ts_.tv_sec) * 1000000000UL + end.tv_nsec - ts_.tv_nsec);
    }
    {
      Operation& operation = instantaneousOperations_[op_];
      ++ operation.callsCount;
      operation.timeNsec += ((end.tv_sec - ts_.tv_sec) * 1000000000UL + end.tv_nsec - ts_.tv_nsec);
    }
    running_ = false;
  }

  static void print()
  {
#ifdef PROFILER
    tbb::spin_rw_mutex::scoped_lock l(guard_, true);
    LOG_CRITICAL(stderr, "\n\n");
    LOG_CRITICAL(stderr, "Stats\n");
    for (const auto& op : operations_) {
      LOG_CRITICAL(stderr, "%20s -> %10lu calls %15lu nsecs %10lu nsecs/call\n", op.first.c_str(), op.second.callsCount, op.second.timeNsec, op.second.timeNsec / op.second.callsCount);
    }
    LOG_CRITICAL(stderr, "Instantaneous Stats\n");
    for (const auto& op : instantaneousOperations_) {
      LOG_CRITICAL(stderr, "%20s -> %10lu calls %15lu nsecs %10lu nsecs/call\n", op.first.c_str(), op.second.callsCount, op.second.timeNsec, op.second.timeNsec / op.second.callsCount);
    }
    instantaneousOperations_.clear();
#endif
  }
};

#ifdef PROFILER
# define START_PROFILER(name) common::TimeProfiler tp(name)
#else
# define START_PROFILER(...)
#endif

#ifdef PROFILER
# define STOP_PROFILER tp.stop()
#else
# define STOP_PROFILER
#endif

} // namespace common
#endif // _PROFILER_H_