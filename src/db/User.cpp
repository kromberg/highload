#include <map>
#include <limits>

#include <rapidjson/stringbuffer.h>

#include "Utils.h"
#include "Location.h"
#include "User.h"

namespace db
{
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

User::User(const rapidjson::Value& jsonVal)
{
  email = std::string(jsonVal["email"].GetString());
  first_name = std::string(jsonVal["first_name"].GetString());
  last_name = std::string(jsonVal["last_name"].GetString());
  birth_date = jsonVal["birth_date"].GetInt();
  gender = *jsonVal["gender"].GetString();
}

User::User(const User& user):
  email(user.email),
  first_name(user.first_name),
  last_name(user.last_name),
  birth_date(user.birth_date),
  gender(user.gender)
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
    tmp.gender = *val.GetString();
    if (tmp.gender != 'f' && tmp.gender != 'm') {
      return false;
    }
  }

  *this = std::move(tmp);

  return true;
}
std::string* User::getJson(int32_t id)
{
  if (!cache_.empty()) {
    return &cache_;
  }
  cache_.reserve(75 + 10 + email.size() + first_name.size() + last_name.size() + 10 + 1 + 16);
  cache_.clear();
  cache_ += "{";
  cache_ += "\"id\":" + std::to_string(id) + ",";
  cache_ += "\"email\":\"" + email + "\",";
  cache_ += "\"first_name\":\"" + first_name + "\",";
  cache_ += "\"last_name\":\"" + last_name + "\",";
  cache_ += "\"birth_date\":" + std::to_string(birth_date) + ",";
  cache_ += "\"gender\":\"" + std::string(1, gender) + "\"";
  cache_ += "}";
  return &cache_;
}

Result User::getJsonVisits(std::string& result, char* params, const int32_t paramsSize) const
{
  struct Parameters
  {
    std::pair<int32_t, int32_t> date{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
    std::string country;
    int32_t toDistance = std::numeric_limits<int32_t>::max();
    bool valid(const Visit& visit) const
    {
      bool res = ((visit.visited_at > date.first) && (visit.visited_at < date.second) &&
              (country.empty() || (country == visit.location_->country)) &&
              (visit.location_->distance < toDistance));
      LOG(stderr, "\n");
      visit.dump();
      dump();
      LOG(stderr, "%s\n", res ? "PASSED" : "FAILED");
      LOG(stderr, "\n");
      
      return res;
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
        uriDecode(requestParameter.country, val + 1, strlen(val + 1));
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
  requestParameter.dump();


  std::multimap<int32_t, Visit*> visits;
  for (const auto& visit : visits_) {
    if (requestParameter.valid(*visit.second)) {
      visits.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(visit.second->visited_at),
        std::forward_as_tuple(visit.second));
    }
  }

  result.reserve(512);
  result.clear();
  result += "{";
  result += "\"visits\":[";
  for (auto visit : visits) {
    result += "{";
    result += "\"mark\":" + std::to_string(visit.second->mark) + ",";
    result += "\"visited_at\":" + std::to_string(visit.second->visited_at) + ",";
    result += "\"place\":\"" + visit.second->location_->place + "\"";
    result += "},";
  }
  if (!visits.empty()) {
    result.pop_back();
  }
  result += "]";
  result += "}";
  return Result::SUCCESS;
}

void User::dump() const
{
  LOG(stderr, "email = %s; first_name = %s; last_name = %s; birth_date = %d; gender = %c\n",
    email.c_str(), first_name.c_str(), last_name.c_str(), birth_date, gender);
}
} // namespace db