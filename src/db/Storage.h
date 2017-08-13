#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <list>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/document.h>

#include <zip.h>

#include <common/Types.h>

#include "User.h"
#include "Location.h"
#include "Visit.h"

namespace db
{
using common::Result;
class Storage
{
private:
  std::unordered_map<int32_t, User> users_;
  tbb::spin_rw_mutex usersGuard_;
  std::unordered_map<int32_t, Location> locations_;
  tbb::spin_rw_mutex locationsGuard_;
  std::unordered_map<int32_t, Visit> visits_;
  tbb::spin_rw_mutex visitsGuard_;

  typedef std::list<struct zip_stat> ZipStats;

private:

  template<class T>
  static void loadFiles(
    std::unordered_map<int32_t, T>& loadMap,
    zip *archive,
    ZipStats& zipStats,
    const std::string& arrayName);
public:
  Result load(const std::string& path);

  Result addUser(const rapidjson::Value& jsonVal);
  Result addLocation(const rapidjson::Value& jsonVal);
  Result addVisit(const rapidjson::Value& jsonVal);

  Result updateUser(const int32_t id, const rapidjson::Value& jsonVal);
  Result updateLocation(const int32_t id, const rapidjson::Value& jsonVal);
  Result updateVisit(const int32_t id, const rapidjson::Value& jsonVal);

  Result getUser(std::string& resp, const int32_t id);
  Result getLocation(std::string& resp, const int32_t id);
  Result getVisit(std::string& resp, const int32_t id);
};

typedef std::shared_ptr<Storage> StoragePtr;

template<class T>
void Storage::loadFiles(
  std::unordered_map<int32_t, T>& loadMap,
  zip *archive,
  ZipStats& zipStats,
  const std::string& arrayName)
{
  char buf[1024];
  LOG(stderr, "Loading to '%s' from %zu file(s)\n", arrayName.c_str(), zipStats.size());
  for (auto& stat : zipStats) {
    LOG(stderr, "Loading from file %s\n", stat.name);

    struct zip_file* zf = zip_fopen(archive, stat.name, 0);
    if (!zf) {
      continue;
    }

    {
      std::ostringstream ss;
      uint64_t sum = 0;
      while (sum != stat.size) {
        int len = zip_fread(zf, buf, sizeof(buf));
        if (len < 0) {
          break;
        }
        ss.write(buf, len);
        sum += len;
      }
      zip_fclose(zf);

      {
        // parse json
        using namespace rapidjson;
        Document document;
        document.Parse(ss.str().c_str());
        if (!document.HasMember(arrayName.c_str())) {
          continue;
        }
        const Value& jsonArr = document[arrayName.c_str()];
        uint64_t loaded = 0;
        for (auto it = jsonArr.Begin(), end = jsonArr.End(); it != end; ++it) {
          try {
            const Value& jsonVal = *it;
            if (!jsonVal.HasMember("id")) {
              continue;
            }
            int32_t id = jsonVal["id"].GetInt();
            loadMap.emplace(
              std::piecewise_construct,
              std::forward_as_tuple(id),
              std::forward_as_tuple(jsonVal));
            ++ loaded;
          } catch (...) {
            LOG(stderr, "Error occurred while parsing entry from '%s'\n", arrayName.c_str());
            continue;
          }
        }
        LOG(stderr, "Loaded %lu entries from %s file\n", loaded, stat.name);
      }
    }
  }
}
} // namespace db
#endif // _STORAGE_H_