#ifndef _DB_USER_H_
#define _DB_USER_H_

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace db
{
struct User
{
  std::string email;
  std::string first_name;
  std::string last_name;
  int64_t birth_date;
  uint16_t gender;

  User(const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
};
} // namespace db
#endif // _DB_USER_H_