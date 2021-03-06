#ifndef _DB_LOCATION_H_
#define _DB_LOCATION_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/document.h>

#include <common/Types.h>

#include "Utils.h"
#include "Visit.h"

namespace db
{
using common::Result;

struct Location
{
  tbb::spin_rw_mutex guard_;

  std::string place;
  std::string country;
  std::string city;
  int32_t distance;

  /*char buffer_[512];
  int bufferSize_ = 0;*/

  std::unordered_map<int32_t, Visit*> visits_;

  Location();
  Location(
    std::string&& _place,
    std::string&& _country,
    std::string&& _city,
    const int32_t _distance);
  Location(
    const int32_t id,
    const rapidjson::Value& jsonVal);
  Location(const Location& location);
  Location& operator=(Location&& location);
  //void cache(const int32_t id);
  void getJson(Buffer& buffer, const int32_t id);
  Result getJsonAvgScore(Buffer& buffer, char* params, const int32_t paramsSize) const;
  void dump() const;
};
} // namespace db
#endif // _DB_LOCATION_H_