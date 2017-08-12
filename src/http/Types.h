#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <rapidjson/document.h>

namespace http
{
enum class Type
{
  NONE,
  POST,
  GET,
};
enum class Table
{
  NONE,
  USERS,
  LOCATIONS,
  VISITS,
  AVG,
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
  std::string params_;

  rapidjson::Document json_;
};
} // namespace http

#endif // _HTTP_TYPES_H_