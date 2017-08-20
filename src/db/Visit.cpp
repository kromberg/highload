#include <rapidjson/stringbuffer.h>

#include "Visit.h"
#include "Location.h"

namespace db
{
Visit::Visit(
  const int32_t locationId,
  const int32_t userId,
  const int32_t _visited_at,
  const uint16_t _mark,
  Location* _location,
  User* _user):
  location(locationId),
  user(userId),
  visited_at(_visited_at),
  mark(_mark),
  location_(_location),
  user_(_user)
{
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

bool Visit::update(const int32_t locationId, const int32_t userId, const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  Visit tmp(*this);
  if (-1 != locationId) {
    tmp.location = locationId;
  }

  if (-1 != userId) {
    tmp.user = userId;
  }

  if (jsonVal.HasMember("visited_at")) {
    const Value& val = jsonVal["visited_at"];
    if (!val.IsInt()) {
      return false;
    }
    tmp.visited_at = val.GetInt();
  }

  if (jsonVal.HasMember("mark")) {
    const Value& val = jsonVal["mark"];
    if (!val.IsInt()) {
      return false;
    }
    tmp.mark = val.GetInt();
  }
  *this = std::move(tmp);
  return true;
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

void Visit::dump() const
{
  LOG(stderr, "id = %d; location = %d, user = %d, visited_at = %d, mark = %d\n",
    id, location, user, visited_at, mark);
}
} // namespace db