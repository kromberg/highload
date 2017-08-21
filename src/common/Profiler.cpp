#include "Profiler.h"

namespace common
{
tbb::spin_rw_mutex TimeProfiler::guard_;
std::unordered_map<std::string, Operation> TimeProfiler::operations_;
} // namespace common
