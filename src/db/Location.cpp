#include <sstream>
#include <iomanip>

#include <rapidjson/stringbuffer.h>

#include "Location.h"

namespace db
{
Location::Location(
  std::string&& _place,
  std::string&& _country,
  std::string&& _city,
  const int32_t _distance):
  place(std::move(_place)),
  country(std::move(_country)),
  city(std::move(_city)),
  distance(_distance)
{}

Location::Location(const rapidjson::Value& jsonVal)
{
  place = std::string(jsonVal["place"].GetString());
  country = std::string(jsonVal["country"].GetString());
  city = std::string(jsonVal["city"].GetString());
  distance = jsonVal["distance"].GetInt();
}

bool Location::update(const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  Location tmp(*this);
  if (jsonVal.HasMember("place")) {
    const Value& val = jsonVal["place"];
    if (!val.IsString()) {
      return false;
    }
    tmp.place = std::string(val.GetString());
  }

  if (jsonVal.HasMember("country")) {
    const Value& val = jsonVal["country"];
    if (!val.IsString()) {
      return false;
    }
    tmp.country = std::string(val.GetString());
  }

  if (jsonVal.HasMember("city")) {
    const Value& val = jsonVal["city"];
    if (!val.IsString()) {
      return false;
    }
    tmp.city = std::string(val.GetString());
  }

  if (jsonVal.HasMember("distance")) {
    const Value& val = jsonVal["distance"];
    if (!val.IsInt()) {
      return false;
    }
    tmp.distance = val.GetInt();
  }

  *this = std::move(tmp);
  return true;
}

std::string Location::getJson(int32_t id)
{
  std::string str;
  str.reserve(512);
  str.clear();
  str += "{";
  str += "\"id\":" + std::to_string(id) + ",";
  str += "\"place\":\"" + place + "\",";
  str += "\"country\":\"" + country + "\",";
  str += "\"city\":\"" + city + "\",";
  str += "\"distance\":" + std::to_string(distance);
  str += "}";
  return str;
}

static std::string to_string(const double val)
{
  std::ostringstream ss;
  ss << std::setprecision(6) << val;
  return ss.str();
}

Result Location::getJsonAvgScore(std::string& result, const char* params, const int32_t paramsSize)
{
  result.reserve(32);
  double avg = 0;
  size_t count = 0;
  for (auto& visitEntry : visits_) {
    Visit* visit = visitEntry.second.second;
    avg += visit->mark;
    ++ count;
  }
  if (count) {
    avg /= count;
  }

  result += "{";
  result += "\"avg\":" + db::to_string(avg);
  result += "}";
  return Result::SUCCESS;
}

} // namespace db