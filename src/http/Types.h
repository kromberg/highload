#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <rapidjson/document.h>

#include <common/Types.h>

namespace http
{
enum class Type
{
  NONE = 0,
  POST,
  GET,
  MAX,
};

enum class Table
{
  NONE = 0,
  USERS,
  LOCATIONS,
  VISITS,
  AVG,
  MAX,
};

enum HTTPCode
{
  NO_ERROR = 0,
  OK = 200,
  BAD_REQ = 400,
  NOT_FOUND = 404,
};

using common::ConstBuffer;
using common::Buffer;

struct Response
{
  ConstBuffer constBuffer;
  Buffer buffer;
  char arr[8 * 1024];
  Response()
  {
    buffer.buffer = arr;
    buffer.capacity = sizeof(arr);
  }
};

struct Request
{
  Type type = Type::NONE;
  Table table1 = Table::NONE;
  Table table2 = Table::NONE;
  int32_t id = -1;
  char* params = nullptr;
  int32_t paramsSize = 0;

  bool hasContentType = false;
  int32_t contentLength = 0;

  const char* content = nullptr;
  bool keepalive = false;
};
} // namespace http

#endif // _HTTP_TYPES_H_