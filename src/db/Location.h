#ifndef _DB_LOCATION_H_
#define _DB_LOCATION_H_

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace db
{
struct Location
{
  std::string place;
  std::string country;
  std::string city;
  int32_t distance;

  Location(const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
};
} // namespace db
#endif // _DB_LOCATION_H_