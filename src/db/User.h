#ifndef _DB_USER_H_
#define _DB_USER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <rapidjson/document.h>

#include "Visit.h"

namespace db
{
struct User
{
  std::string email;
  std::string first_name;
  std::string last_name;
  int64_t birth_date;
  uint16_t gender;

  std::unordered_map<int32_t, Visit*> visits_;

  User(const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
  std::string getJsonVisits(std::string& params);
};
} // namespace db
#endif // _DB_USER_H_