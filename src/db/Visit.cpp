#include <rapidjson/stringbuffer.h>

#include "Visit.h"
#include "Location.h"
#include "User.h"

namespace db
{
Visit::Visit()
{}

Visit::Visit(
  const int32_t id,
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
  cache(id);
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
  bufferSize_ = 0;
  return *this;
}

void Visit::cache(const int32_t id)
{
  int size =
    snprintf(buffer_ + DB_RESPONSE_200_SIZE, sizeof(buffer_) - DB_RESPONSE_200_SIZE,
      "{\"id\":%d,\"location\":%d,\"user\":%d,\"visited_at\":%d,\"mark\":%u}",
      id, location, user, visited_at, mark);
  bufferSize_ = snprintf(buffer_, DB_RESPONSE_200_SIZE, DB_RESPONSE_200, size);
  buffer_[bufferSize_ - 1] = '\n';
  bufferSize_ += size;
}

void Visit::getJson(ConstBuffer& buffer, const int32_t id)
{
  if (0 == bufferSize_) {
    cache(id);
  }
  buffer.buffer = buffer_;
  buffer.size = bufferSize_;
}

void Visit::dump() const
{
  LOG(stderr, "location = %d, user = %d, visited_at = %d, mark = %d\n",
    location, user, visited_at, mark);
  user_->dump();
  location_->dump();
}
} // namespace db