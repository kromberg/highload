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
} // namespace common

#if 1
# define LOG fprintf
#else
# define LOG(...) (void)
#endif

#endif // _TYPES_H_