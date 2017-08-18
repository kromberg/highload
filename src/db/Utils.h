#ifndef _DB_UTILS_H_
#define _DB_UTILS_H_

namespace db
{

#define PARSE_INT32(val, buffer) \
{\
  char* end;\
  val = static_cast<int32_t>(strtol((buffer), &end, 10));\
  if (0 == end) {\
    return Result::FAILED;\
  }\
}

} // namespace db
#endif // _DB_UTILS_H_