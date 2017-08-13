#include <rapidjson/stringbuffer.h>

#include "User.h"

namespace db
{
User::User(const rapidjson::Value& jsonVal)
{
  email = std::string(jsonVal["email"].GetString());
  first_name = std::string(jsonVal["first_name"].GetString());
  last_name = std::string(jsonVal["last_name"].GetString());
  birth_date = jsonVal["birth_date"].GetInt64();
  const char* genderStr = jsonVal["gender"].GetString();
  if (*genderStr == 'f') {
    gender = false;
  } else if (*genderStr == 'm') {
    gender = true;
  }
}

void User::update(const rapidjson::Value& jsonVal)
{
  if (jsonVal.HasMember("email")) {
    email = std::string(jsonVal["email"].GetString());
  }
  if (jsonVal.HasMember("first_name")) {
    first_name = std::string(jsonVal["first_name"].GetString());
  }
  if (jsonVal.HasMember("last_name")) {
    last_name = std::string(jsonVal["last_name"].GetString());
  }
  if (jsonVal.HasMember("birth_date")) {
    birth_date = jsonVal["birth_date"].GetInt64();
  }
  if (jsonVal.HasMember("gender")) {
    const char* genderStr = jsonVal["gender"].GetString();
    if (*genderStr == 'f') {
      gender = false;
    } else if (*genderStr == 'm') {
      gender = true;
    }
  }
}
std::string User::getJson(int32_t id)
{
  thread_local std::string str;
  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"id\":" + std::to_string(id) + ",";
  str += "\"email\":\"" + email + "\",";
  str += "\"first_name\":\"" + first_name + "\",";
  str += "\"last_name\":\"" + last_name + "\",";
  str += "\"birth_date\":" + std::to_string(birth_date) + ",";
  str += "\"gender\":\"" + std::string(gender ? "m" : "f") + "\",";
  str += "}";
  return str;
}
} // namespace db