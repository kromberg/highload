#include <rapidjson/stringbuffer.h>

#include "Location.h"

namespace db
{
Location::Location(const rapidjson::Value& jsonVal)
{
  place = std::string(jsonVal["place"].GetString());
  country = std::string(jsonVal["country"].GetString());
  city = std::string(jsonVal["city"].GetString());
  distance = jsonVal["distance"].GetInt();
}

void Location::update(const rapidjson::Value& jsonVal)
{
  if (jsonVal.HasMember("place")) {
    place = std::string(jsonVal["place"].GetString());
  }
  if (jsonVal.HasMember("country")) {
    country = std::string(jsonVal["country"].GetString());
  }
  if (jsonVal.HasMember("city")) {
    city = std::string(jsonVal["city"].GetString());
  }
  if (jsonVal.HasMember("distance")) {
    distance = jsonVal["distance"].GetInt();
  }
}
std::string Location::getJson(int32_t id)
{
  thread_local std::string str;
  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"id\":" + std::to_string(id) + ",";
  str += "\"place\":\"" + place + "\",";
  str += "\"country\":\"" + country + "\",";
  str += "\"city\":\"" + city + "\",";
  str += "\"distance\":" + std::to_string(distance) + ",";
  str += "}";
  return str;
}
} // namespace db