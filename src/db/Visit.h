#ifndef _DB_VISIT_H_
#define _DB_VISIT_H_

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace db
{
struct Visit
{
  int32_t location;
  int32_t user;
  int32_t visited_at;
  uint16_t mark;

  Visit(const rapidjson::Value& jsonVal);
  void update(const rapidjson::Value& jsonVal);
  std::string getJson(int32_t id);
};
} // namespace db
#endif // _DB_VISIT_H_