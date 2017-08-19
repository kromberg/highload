#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <rapidjson/document.h>

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
  OK = 200,
  BAD_REQ = 400,
  NOT_FOUND = 404,
};

inline const char* httpCodeToStr(const HTTPCode code)
{
  switch (code) {
    case HTTPCode::OK:
      return "OK";
    case HTTPCode::BAD_REQ:
      return "Bad Request";
    case HTTPCode::NOT_FOUND:
      return "Not Found";
  }
  return "Unknown Error";
}

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

  rapidjson::Document json_;

  void dump() {
    LOG(stderr, "Type = %d; Table1 = %d; Table2 = %d; ID = %d; Params = %s\n",
      type_, table1_, table2_, id_, params_);
  }
};
} // namespace http

#endif // _HTTP_TYPES_H_