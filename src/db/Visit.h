#ifndef _DB_VISIT_H_
#define _DB_VISIT_H_

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace db
{
struct User;
struct Location;
struct Visit
{
  int32_t location;
  int32_t user;
  int32_t visited_at;
  uint16_t mark;

  Location* location_;
  User* user_;

  Visit(const rapidjson::Value& jsonVal);
  Visit(
    const int32_t locationId,
    const int32_t userId,
    Location* _location,
    User* _user,
    const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
};
} // namespace db
#endif // _DB_VISIT_H_