#ifndef _TYPES_H_
#define _TYPES_H_

namespace common
{
enum class Result
{
  SUCCESS,
  FAILED,
  NOT_FOUND,
};

struct ConstBuffer
{
  const char* buffer = nullptr;
  int size = 0;
};

struct Buffer
{
  char* buffer = nullptr;
  int capacity = 0;
  int size = 0;
};

} // namespace common

#ifndef NDEBUG
# define LOG fprintf
#else
# define LOG(...)
#endif

#define LOG_CRITICAL fprintf

#endif // _TYPES_H_