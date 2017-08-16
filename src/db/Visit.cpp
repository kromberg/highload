#include <rapidjson/stringbuffer.h>

#include "Visit.h"
#include "Location.h"

namespace db
{
Visit::Visit(const rapidjson::Value& jsonVal)
{
  location = jsonVal["location"].GetInt();
  user = jsonVal["user"].GetInt();
  visited_at = jsonVal["visited_at"].GetInt();
  mark = jsonVal["mark"].GetInt();
}

Visit::Visit(
  const int32_t locationId,
  const int32_t userId,
  Location* _location,
  User* _user,
  const rapidjson::Value& jsonVal):
  location(locationId),
  user(userId),
  location_(_location),
  user_(_user)
{
  visited_at = jsonVal["visited_at"].GetInt();
  mark = jsonVal["mark"].GetInt();
}

void Visit::update(const rapidjson::Value& jsonVal)
{
  if (jsonVal.HasMember("location")) {
    location = jsonVal["location"].GetInt();
  }
  if (jsonVal.HasMember("user")) {
    user = jsonVal["user"].GetInt();
  }
  if (jsonVal.HasMember("visited_at")) {
    visited_at = jsonVal["visited_at"].GetInt();
  }
  if (jsonVal.HasMember("mark")) {
    mark = jsonVal["mark"].GetInt();
  }
}
std::string Visit::getJson(int32_t id)
{
  std::string str;
  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"id\":" + std::to_string(id) + ",";
  str += "\"location\":" + std::to_string(location) + ",";
  str += "\"user\":" + std::to_string(user) + ",";
  str += "\"visited_at\":" + std::to_string(visited_at) + ",";
  str += "\"mark\":" + std::to_string(mark);
  str += "}";
  return str;
}
} // namespace db