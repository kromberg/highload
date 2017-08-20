#include <rapidjson/stringbuffer.h>

#include "Visit.h"
#include "Location.h"
#include "User.h"

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

Visit::Visit(const Visit& visit):
  location(visit.location),
  user(visit.user),
  visited_at(visit.visited_at),
  mark(visit.mark),
  location_(visit.location_),
  user_(visit.user_)
{}

Visit& Visit::operator=(Visit&& visit)
{
  location = std::move(visit.location);
  user = std::move(visit.user);
  visited_at = std::move(visit.visited_at);
  mark = std::move(visit.mark);
  location_ = std::move(visit.location_);
  user_ = std::move(visit.user_);
  return *this;
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

std::string* Visit::getJson(int32_t id)
{
  if (!cache_.empty()) {
    return &cache_;
  }
  cache_.reserve(512);
  cache_.clear();
  cache_ += "{";
  cache_ += "\"id\":" + std::to_string(id) + ",";
  cache_ += "\"location\":" + std::to_string(location) + ",";
  cache_ += "\"user\":" + std::to_string(user) + ",";
  cache_ += "\"visited_at\":" + std::to_string(visited_at) + ",";
  cache_ += "\"mark\":" + std::to_string(mark);
  cache_ += "}";
  return &cache_;
}

void Visit::dump() const
{
  LOG(stderr, "location = %d, user = %d, visited_at = %d, mark = %d\n",
    location, user, visited_at, mark);
  user_->dump();
  location_->dump();
}
} // namespace db