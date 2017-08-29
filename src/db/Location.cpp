#include <sstream>
#include <iomanip>
#include <ctime>
#include <time.h>
#include <cmath>

#include <rapidjson/stringbuffer.h>

#include "Utils.h"
#include "Location.h"
#include "Storage.h"

namespace db
{
Location::Location()
{}

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

Location::Location(
  const int32_t id,
  const rapidjson::Value& jsonVal)
{
  place = std::string(jsonVal["place"].GetString());
  country = std::string(jsonVal["country"].GetString());
  city = std::string(jsonVal["city"].GetString());
  distance = jsonVal["distance"].GetInt();
}

Location::Location(const Location& location):
  place(location.place),
  country(location.country),
  city(location.city),
  distance(location.distance)
{}

Location& Location::operator=(Location&& location)
{
  place = std::move(location.place);
  country = std::move(location.country);
  city = std::move(location.city);
  distance = std::move(location.distance);
  return *this;
}

void Location::getJson(Buffer& buffer, const int32_t id)
{
  int size =
    snprintf(buffer.buffer + DB_RESPONSE_200_SIZE, buffer.capacity - DB_RESPONSE_200_SIZE,
      "{\"id\":%d,\"place\":\"%s\",\"country\":\"%s\",\"city\":\"%s\",\"distance\":%d}",
      id, place.c_str(), country.c_str(), city.c_str(), distance);
  buffer.size = snprintf(buffer.buffer, DB_RESPONSE_200_SIZE, DB_RESPONSE_200, size);
  buffer.buffer[buffer.size - 1] = '\n';
  buffer.size += size;
}

Result Location::getJsonAvgScore(Buffer& buffer, char* params, const int32_t paramsSize) const
{
  struct Parameters
  {
    std::pair<int32_t, int32_t> date{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    std::pair<int32_t, int32_t> birth_date{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    char gender = 0;
    bool valid(const Visit& visit) const
    {
      return (visit.visited_at > date.first && visit.visited_at < date.second &&
              visit.user_->birth_date > birth_date.first && visit.user_->birth_date < birth_date.second &&
              (!gender || visit.user_->gender == gender));
    }
    void dump() const
    {
      LOG(stderr, "date.first = %d\ndate.second = %d\nbirth_date.first = %d\nbirth_date.second = %d\ngender = %c\n",
        date.first, date.second, birth_date.first, birth_date.second, gender);
    }
  } requestParameter;
  if (params) {
    // parse parameters
    char* next = nullptr, *param = params;
    do {
      next = strchr(param, '&');
      char *paramEnd;
      if (next) {
        *next = '\0';
        paramEnd = next;
      } else {
        paramEnd = params + paramsSize;
      }
      // parse parameter
      LOG(stderr, "Parsing parameter %s\n", param);
      char* val = strchr(param, '=');
      if (!val) {
        return Result::FAILED;
      }
      if (0 == strncmp(param, "fromDate", val - param)) {
        PARSE_INT32(requestParameter.date.first, val + 1, paramEnd);
      } else if (0 == strncmp(param, "toDate", val - param)) {
        PARSE_INT32(requestParameter.date.second, val + 1, paramEnd);
      } else if (0 == strncmp(param, "gender", val - param)) {
        requestParameter.gender = *(val + 1);
        if ('f' != requestParameter.gender && 
            'm' != requestParameter.gender) {
          return Result::FAILED;
        }
      } else if (0 == strncmp(param, "fromAge", val - param)) {
        int32_t fromAge;
        PARSE_INT32(fromAge, val + 1, paramEnd);
        struct tm fromTime = Storage::getTime();
        fromTime.tm_year -= fromAge;
        requestParameter.birth_date.second = static_cast<int32_t>(timegm(&fromTime));
        LOG(stderr, "to birth_date = %s\n", asctime(&fromTime));
      } else if (0 == strncmp(param, "toAge", val - param)) {
        int32_t toAge;
        PARSE_INT32(toAge, val + 1, paramEnd);
        struct tm toTime = Storage::getTime();
        toTime.tm_year -= toAge;
        requestParameter.birth_date.first = static_cast<int32_t>(timegm(&toTime));
        LOG(stderr, "from birth_date = %s\n", asctime(&toTime));
      } else {
        return Result::FAILED;
      }

      if (next) {
        param = next + 1;
      }
    } while (next);
  }

  requestParameter.dump();

  uint64_t sum = 0;
  uint64_t count = 0;

  for (const auto& visit : visits_) {
    if (requestParameter.valid(*visit.second)) {
      ++ count;
      sum += visit.second->mark;
    }
  }

  long double avg = 0;
  if (count) {
    avg = roundl(((long double)(sum * 100000)) / count) / 100000;
  }

  int size =
    snprintf(buffer.buffer + DB_RESPONSE_200_SIZE, buffer.capacity - DB_RESPONSE_200_SIZE,
      "{\"avg\":%.5Lf}", avg);
  buffer.size = snprintf(buffer.buffer, DB_RESPONSE_200_SIZE, DB_RESPONSE_200, size);
  buffer.buffer[buffer.size - 1] = '\n';
  buffer.size += size;
  return Result::SUCCESS;
}
void Location::dump() const
{
  LOG(stderr, "place = %s; country = %s; city = %s; distance = %d\n",
    place.c_str(), country.c_str(), city.c_str(), distance);
}
} // namespace db