#ifndef _DB_USER_H_
#define _DB_USER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <rapidjson/document.h>

#include <common/Types.h>

#include "Visit.h"

namespace db
{
using common::Result;

struct User
{
  std::string email;
  std::string first_name;
  std::string last_name;
  int64_t birth_date;
  uint16_t gender;

  std::unordered_map<int32_t, Visit*> visits_;

  User(
    std::string&& _email,
    std::string&& _first_name,
    std::string&& _last_name,
    const int32_t _birth_date,
    const uint16_t _gender);
  User(const rapidjson::Value& jsonVal);
  bool update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
  Result getJsonVisits(std::string& result, char* params, const int32_t paramsSize);
};
} // namespace db
#endif // _DB_USER_H_