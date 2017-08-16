#ifndef _DB_LOCATION_H_
#define _DB_LOCATION_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <rapidjson/document.h>

#include "User.h"
#include "Visit.h"

namespace db
{
struct Location
{
  std::string place;
  std::string country;
  std::string city;
  int32_t distance;

  std::unordered_map<int32_t, std::pair<User*, Visit*> > visits_;

  Location(const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
  std::string getJsonAvgMark(std::string& params);
};
} // namespace db
#endif // _DB_LOCATION_H_