#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <rapidjson/document.h>

#include <db/Utils.h>

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

struct Response
{
  db::ConstBuffer constBuffer;
  db::Buffer buffer;
  char arr[8 * 1024];
  Response()
  {
    buffer.buffer = arr;
    buffer.capacity = sizeof(arr);
  }
};

struct Request
{
  Type type_ = Type::NONE;
  Table table1_ = Table::NONE;
  Table table2_ = Table::NONE;
  int32_t id_ = -1;
  char* params_ = nullptr;
  int32_t paramsSize_ = 0;

  bool hasContentType_ = false;
  int32_t contentLength_ = 0;

  const char* content_ = nullptr;
  bool keepalive_ = false;
};
} // namespace http

#endif // _HTTP_TYPES_H_