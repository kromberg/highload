#ifndef _DB_VISIT_H_
#define _DB_VISIT_H_

#include <cstdint>
#include <string>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/document.h>

namespace db
{
struct User;
struct Location;
struct Visit
{
  tbb::spin_rw_mutex guard_;

  int32_t location;
  int32_t user;
  int32_t visited_at;
  uint16_t mark;

  std::string cache_;

  Location* location_;
  User* user_;

  Visit(
    const int32_t locationId,
    const int32_t userId,
    const int32_t _visited_at,
    const uint16_t _mark,
    Location* _location,
    User* _user);
  Visit(
    const int32_t id,
    const int32_t locationId,
    const int32_t userId,
    Location* _location,
    User* _user,
    const rapidjson::Value& jsonVal);
  Visit(const Visit& visit);
  Visit& operator=(Visit&& visit);
  void cache(const int32_t id);
  bool update(const int32_t locationId, const int32_t userId, const rapidjson::Value& jsonVal);
  std::string* getJson(const int32_t id);
  void dump() const;
};
} // namespace db
#endif // _DB_VISIT_H_