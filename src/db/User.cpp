#include <map>
#include <limits>

#include <rapidjson/stringbuffer.h>

#include "Utils.h"
#include "Location.h"
#include "User.h"

namespace db
{
User::User()
{}

User::User(
  std::string&& _email,
  std::string&& _first_name,
  std::string&& _last_name,
  const int32_t _birth_date,
  const char _gender):
  email(std::move(_email)),
  first_name(std::move(_first_name)),
  last_name(std::move(_last_name)),
  birth_date(_birth_date),
  gender(_gender)
{}

User::User(
  const int32_t id,
  const rapidjson::Value& jsonVal)
{
  email = std::string(jsonVal["email"].GetString());
  first_name = std::string(jsonVal["first_name"].GetString());
  last_name = std::string(jsonVal["last_name"].GetString());
  birth_date = jsonVal["birth_date"].GetInt();
  gender = *jsonVal["gender"].GetString();
  //cache(id);
}

User::User(const User& user):
  email(user.email),
  first_name(user.first_name),
  last_name(user.last_name),
  birth_date(user.birth_date),
  gender(user.gender)
{}

User::User(User&& user):
  email(std::move(user.email)),
  first_name(std::move(user.first_name)),
  last_name(std::move(user.last_name)),
  birth_date(std::move(user.birth_date)),
  gender(std::move(user.gender))
{}

User& User::operator=(User&& user)
{
  email = std::move(user.email);
  first_name = std::move(user.first_name);
  last_name = std::move(user.last_name);
  birth_date = std::move(user.birth_date);
  gender = std::move(user.gender);
  return *this;
}

void User::getJson(Buffer& buffer, const int32_t id)
{
  int size =
    snprintf(buffer.buffer + DB_RESPONSE_200_SIZE, buffer.capacity - DB_RESPONSE_200_SIZE,
      "{\"id\":%d,\"email\":\"%s\",\"first_name\":\"%s\",\"last_name\":\"%s\",\"birth_date\":%d,\"gender\":\"%c\"}",
      id, email.c_str(), first_name.c_str(), last_name.c_str(), birth_date, gender);
  buffer.size = snprintf(buffer.buffer + DB_RESPONSE_200_PART1_SIZE, DB_RESPONSE_200_PART2_SIZE, DB_RESPONSE_200_PART2, size);
  buffer.size += DB_RESPONSE_200_PART1_SIZE;
  buffer.buffer[buffer.size - 1] = '\n';
  buffer.size += size;
}

Result User::getJsonVisits(Buffer& buffer, char* params, const int32_t paramsSize) const
{
  static constexpr size_t COUNTRY_CAPACITY = 51;
  thread_local char country[COUNTRY_CAPACITY];
  thread_local size_t countrySize;
  struct Parameters
  {
    std::pair<int32_t, int32_t> date{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    bool countryToLong = false;
    in_place_string country;
    int32_t toDistance = std::numeric_limits<int32_t>::max();
    bool valid(const Visit& visit) const
    {
      return ((visit.visited_at > date.first) && (visit.visited_at < date.second) &&
              (country.empty() || (country == visit.location_->country)) &&
              (visit.location_->distance < toDistance));
    }
    void dump() const
    {
      LOG(stderr, "date.first = %d\ndate.second = %d\ncountry = %s\ndistance = %d\n",
        date.first, date.second, country.c_str(), toDistance);
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
      } else if (0 == strncmp(param, "country", val - param)) {
        if (!uriDecode(country, countrySize, COUNTRY_CAPACITY, val + 1, strlen(val + 1))) {
          requestParameter.countryToLong = true;
        } else {
          requestParameter.country = in_place_string(country, countrySize);
        }
      } else if (0 == strncmp(param, "toDistance", val - param)) {
        PARSE_INT32(requestParameter.toDistance, val + 1, paramEnd);
      } else {
        return Result::FAILED;
      }

      if (next) {
        param = next + 1;
      }
    } while (next);
  }

  int offset = DB_RESPONSE_200_SIZE;
  int size = 0;

  if (requestParameter.countryToLong) {
    size = snprintf(buffer.buffer + offset, buffer.capacity - offset, "{\"visits\":[]}");
    offset += size;
  } else {

  std::multimap<int32_t, Visit*> visits;
    for (const auto& visit : visits_) {
      if (requestParameter.valid(*visit.second)) {
        visits.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(visit.second->visited_at),
          std::forward_as_tuple(visit.second));
      }
    }

    size = snprintf(buffer.buffer + offset, buffer.capacity - offset, "{\"visits\":[");
    offset += size;

    for (auto visit : visits) {
      size = snprintf(buffer.buffer + offset, buffer.capacity - offset, "{\"mark\":%d,\"visited_at\":%d,\"place\":\"%s\"},",
        visit.second->mark, visit.second->visited_at, visit.second->location_->place.c_str());
      offset += size;
    }
    if (!visits.empty()) {
      -- offset;
    }
    size = snprintf(buffer.buffer + offset, buffer.capacity - offset, "]}");
    offset += size;
  }

  buffer.size = snprintf(buffer.buffer + DB_RESPONSE_200_PART1_SIZE, DB_RESPONSE_200_PART2_SIZE, DB_RESPONSE_200_PART2, offset - DB_RESPONSE_200_SIZE);
  buffer.size += DB_RESPONSE_200_PART1_SIZE;
  buffer.buffer[buffer.size - 1] = '\n';
  buffer.size = offset;
  return Result::SUCCESS;
}

void User::dump() const
{
  LOG(stderr, "email = %s; first_name = %s; last_name = %s; birth_date = %d; gender = %c\n",
    email.c_str(), first_name.c_str(), last_name.c_str(), birth_date, gender);
}
} // namespace db