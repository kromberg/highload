#ifndef _TYPES_H_
#define _TYPES_H_

#include <cstring>
#include <functional>

namespace common
{
enum class Result
{
  SUCCESS,
  FAILED,
  NOT_FOUND,
  AGAIN,
  CLOSE,
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

struct in_place_string
{
  const char* buffer_;
  size_t size_;
  in_place_string():
    buffer_(nullptr), size_(0)
  {}
  in_place_string(const char* buffer):
    buffer_(buffer), size_(strlen(buffer))
  {}
  in_place_string(const char* buffer, size_t size):
    buffer_(buffer), size_(size)
  {}
  in_place_string(const std::string& str):
    buffer_(str.c_str()), size_(str.size())
  {}
  bool empty() const
  {
    return 0 == size;
  }
  bool operator==(const in_place_string& rhs) const
  {
    if (size_ != rhs.size_) {
      return false;
    }
    return (0 == strncmp(buffer_, rhs.buffer_, size_));
  }
  bool operator==(const std::string& rhs) const
  {
    if (size_ != rhs.size()) {
      return false;
    }
    return (0 == strncmp(buffer_, rhs.c_str(), size_));
  }
};

template<>
struct std::hash<in_place_string>
{
  size_t operator()(const in_place_string& str) const
  {
    size_t result = 0;
    const size_t prime = 31;
    for (size_t i = 0; i < str.size_; ++i) {
      result = str.buffer_[i] + (result * prime);
    }
    return result;
  }
};

#ifndef NDEBUG
# define LOG fprintf
#else
# define LOG(...)
#endif

#define LOG_CRITICAL fprintf

#endif // _TYPES_H_