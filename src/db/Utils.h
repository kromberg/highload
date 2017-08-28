#ifndef _DB_UTILS_H_
#define _DB_UTILS_H_

#include <common/Types.h>

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

#define DB_RESPONSE_200 \
  "HTTP/1.1 200 OK\n"\
  "Content-Type: application/json; charset=UTF-8\n"\
  "Connection: keep-alive\n"\
  "Content-Length: %5d\n"\
  "\n"
#define DB_RESPONSE_200_SIZE 108

using common::ConstBuffer;
using common::Buffer;

inline void uriDecode(std::string& res, char* str, int32_t size)
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

  res.resize(size);
  size_t resSize = 0;
  char * pEnd = &res[0];

  while (pSrc < SRC_LAST_DEC)
  {
    if (*pSrc == '%')
    {
      char dec1, dec2;
      if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)]) &&
          -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
      {
        *pEnd++ = (dec1 << 4) + dec2;
        pSrc += 3;
        ++ resSize;
        continue;
      }
    } else if (*pSrc == '+') {
      *pEnd = ' ';
      ++ pEnd;
      ++ pSrc;
      ++ resSize;
      continue;
    }


    *pEnd++ = *pSrc++;
    ++ resSize;
  }

     // the last 2- chars
  while (pSrc < SRC_END) {
    *pEnd++ = *pSrc++;
    ++ resSize;
  }

  res.resize(resSize);
}

} // namespace db
#endif // _DB_UTILS_H_