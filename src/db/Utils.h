#ifndef _DB_UTILS_H_
#define _DB_UTILS_H_

#include <common/Types.h>
#include <common/Profiler.h>

namespace db
{

#define PARSE_INT32(val, buffer, bufferEnd) \
{\
  char* end;\
  val = static_cast<int32_t>(strtol((buffer), &end, 10));\
  if (bufferEnd != end) {\
    return Result::FAILED;\
  }\
}

#define DB_RESPONSE_200_PART2 \
  "%5d\n"\
  "\n"
#define DB_RESPONSE_200_SIZE 108
#define DB_RESPONSE_200_PART1_SIZE 101
#define DB_RESPONSE_200_PART2_SIZE 7

using common::ConstBuffer;
using common::Buffer;

inline bool uriDecode(char* res, size_t& outSize, const size_t capacity, char* str, int32_t size)
{
   // Note from RFC1630: "Sequences which start with a percent
   // sign but are not followed by two hexadecimal characters
   // (0-9, A-F) are reserved for future extension"
  static const char HEX2DEC[256] = 
  {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    
    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    
    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    
    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
  };
  const unsigned char * pSrc = (const unsigned char *)str;
  const unsigned char * const SRC_END = pSrc + size;
   // last decodable '%' 
  const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

  outSize = 0;
  char * pEnd = res;

  while (pSrc < SRC_LAST_DEC && outSize < capacity)
  {
    if (*pSrc == '%')
    {
      char dec1, dec2;
      if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)]) &&
          -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
      {
        *pEnd++ = (dec1 << 4) + dec2;
        pSrc += 3;
        ++ outSize;
        continue;
      }
    } else if (*pSrc == '+') {
      *pEnd = ' ';
      ++ pEnd;
      ++ pSrc;
      ++ outSize;
      continue;
    }

    *pEnd++ = *pSrc++;
    ++ outSize;
  }

  // the last 2- chars
  while (pSrc < SRC_END && outSize < capacity) {
    *pEnd++ = *pSrc++;
    ++ outSize;
  }

  if (pSrc < SRC_END) {
    return false;
  }

  return true;
}

} // namespace db
#endif // _DB_UTILS_H_