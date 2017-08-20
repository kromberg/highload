#ifndef _DB_USER_H_
#define _DB_USER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/document.h>

#include <common/Types.h>

#include "Visit.h"

namespace db
{
using common::Result;

struct User
{
  tbb::spin_rw_mutex guard_;

  std::string email;
  std::string first_name;
  std::string last_name;
  int32_t birth_date;
  char gender;

  std::string cache_;

  std::unordered_map<int32_t, Visit*> visits_;

  User(
    std::string&& _email,
    std::string&& _first_name,
    std::string&& _last_name,
    const int32_t _birth_date,
    const char _gender);
  User(const rapidjson::Value& jsonVal);
  User(const User& user);
  User& operator=(User&& user);
  bool update(const rapidjson::Value& jsonVal);
  std::string* getJson(int32_t id);
  Result getJsonVisits(std::string& result, char* params, const int32_t paramsSize) const;
  void dump() const;
};
} // namespace db
#endif // _DB_USER_H_