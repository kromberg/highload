#include <rapidjson/stringbuffer.h>

#include "Location.h"
#include "User.h"

namespace db
{
User::User(
  std::string&& _email,
  std::string&& _first_name,
  std::string&& _last_name,
  const int32_t _birth_date,
  const uint16_t _gender):
  email(std::move(_email)),
  first_name(std::move(_first_name)),
  last_name(std::move(_last_name)),
  birth_date(_birth_date),
  gender(_gender)
{}

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

bool User::update(const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  User tmp(*this);
  if (jsonVal.HasMember("email")) {
    const Value& val = jsonVal["email"];
    if (!val.IsString()) {
      return false;
    }
    tmp.email = std::string(val.GetString());
  }

  if (jsonVal.HasMember("first_name")) {
    const Value& val = jsonVal["first_name"];
    if (!val.IsString()) {
      return false;
    }
    tmp.first_name = std::string(val.GetString());
  }

  if (jsonVal.HasMember("last_name")) {
    const Value& val = jsonVal["last_name"];
    if (!val.IsString()) {
      return false;
    }
    tmp.last_name = std::string(val.GetString());
  }

  if (jsonVal.HasMember("birth_date")) {
    const Value& val = jsonVal["birth_date"];
    if (!val.IsInt()) {
      return false;
    }
    tmp.birth_date = val.GetInt();
  }

  if (jsonVal.HasMember("gender")) {
    const Value& val = jsonVal["gender"];
    if (!val.IsString()) {
      return false;
    }
    const char* genderStr = val.GetString();
    if (*genderStr == 'f') {
      tmp.gender = false;
    } else if (*genderStr == 'm') {
      tmp.gender = true;
    }
  }

  *this = std::move(tmp);

  return true;
}
std::string User::getJson(int32_t id)
{
  std::string str;
  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"id\":" + std::to_string(id) + ",";
  str += "\"email\":\"" + email + "\",";
  str += "\"first_name\":\"" + first_name + "\",";
  str += "\"last_name\":\"" + last_name + "\",";
  str += "\"birth_date\":" + std::to_string(birth_date) + ",";
  str += "\"gender\":\"" + std::string(gender ? "m" : "f") + "\"";
  str += "}";
  return str;
}

std::string User::getJsonVisits(const char* params, const int32_t paramsSize)
{
  std::string str;

  struct Parameters
  {
    std::pair<int32_t, int32_t> date;
  };
  // TODO: parse parameters

  if (visits_.empty()) {
    return std::string();
  }

  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"visits\":[";
  for (auto visit : visits_) {
    str += "{";
    str += "\"mark\":" + std::to_string(visit.second->mark) + ",";
    str += "\"visited_at\":" + std::to_string(visit.second->visited_at) + ",";
    str += "\"place\":\"" + visit.second->location_->place + "\"";
    str += "},";
  }
  str.pop_back();
  str += "]";
  str += "}";
  return str;
}
} // namespace db