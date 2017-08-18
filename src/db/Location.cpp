#include <sstream>
#include <iomanip>
#include <ctime>
#include <time.h>

#include <rapidjson/stringbuffer.h>

#include "Utils.h"
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

Result Location::getJsonAvgScore(std::string& result, char* params, const int32_t paramsSize)
{
  static time_t currentTimeT = time(nullptr);
  static struct tm currentTime = *gmtime(&currentTimeT);
  struct Parameters
  {
    std::pair<int32_t, int32_t> date{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    std::pair<int32_t, int32_t> age{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    char gender = 0;
    bool valid(const std::pair<User*, Visit*>& visit) const
    {
      return (visit.second->visited_at > date.first && visit.second->visited_at < date.second &&
              visit.first->birth_date > age.first && visit.first->birth_date < age.second &&
              (!gender || visit.first->gender == gender));
    }
  } requestParameter;
  // parse parameters
  char* next = nullptr, *param = params;
  do {
    char* next = strchr(param, '&');
    if (next) {
      *next = '\0';
    }
    // parse parameter
    char* val = strchr(param, '=');
    if (!val) {
      return Result::FAILED;
    }
    if (0 == strncmp(param, "fromDate", val - param)) {
      PARSE_INT32(requestParameter.date.first, val + 1);
    } else if (0 == strncmp(param, "toDate", val - param)) {
      PARSE_INT32(requestParameter.date.second, val + 1);
    } else if (0 == strncmp(param, "gender", val - param)) {
      requestParameter.gender = *(val + 1);
    } else if (0 == strncmp(param, "fromAge", val - param)) {
      int32_t fromAge;
      PARSE_INT32(fromAge, val + 1);
      struct tm fromTime = currentTime;
      fromTime.tm_year -= fromAge;
      requestParameter.age.first = static_cast<int32_t>(timegm(&fromTime));
    } else if (0 == strncmp(param, "toAge", val - param)) {
      int32_t toAge;
      PARSE_INT32(toAge, val + 1);
      struct tm toTime = currentTime;
      toTime.tm_year -= toAge;
      requestParameter.age.second = static_cast<int32_t>(timegm(&toTime));
    } else {
      return Result::FAILED;
    }

    if (next) {
      param = next + 1;
    }
  } while (next);

  uint64_t sum = 0;
  uint64_t count = 0;

  for (const auto& visit : visits_) {
    if (requestParameter.valid(visit.second)) {
      ++ count;
      sum += visit.second.second->mark;
    }
  }

  double avg = 0;
  if (count) {
    avg = (1.0 * sum) / count;
  }

  result.reserve(32);
  result += "{";
  result += "\"avg\":" + db::to_string(avg);
  result += "}";
  return Result::SUCCESS;
}

} // namespace db