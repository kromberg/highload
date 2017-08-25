#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <list>
#include <ctime>
#include <time.h>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/document.h>

#include <zip.h>

#include <common/Types.h>

#include "Utils.h"
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

  static struct tm time_;

  typedef std::list<struct zip_stat> ZipStats;
  typedef std::function<void(const rapidjson::Value&)> MapLoaderFunc;

private:

  static void loadFiles(
    zip *archive,
    ZipStats& zipStats,
    const std::string& arrayName,
    const MapLoaderFunc& func);
public:
  Result load(const std::string& path);
  static Result loadTime(const std::string& path);
  static struct tm getTime();

  Result addUser(const char* content);
  Result updateUser(const int32_t id, const char* content);
  Result getUser(ConstBuffer& buffer, const int32_t id);
  Result getUserVisits(Buffer& buffer, const int32_t id, char* params, const int32_t paramsSize);

  Result addLocation(const char* content);
  Result updateLocation(const int32_t id, const char* content);
  Result getLocation(ConstBuffer& buffer, const int32_t id);
  Result getLocationAvgScore(Buffer& buffer, const int32_t id, char* params, const int32_t paramsSize);

  Result addVisit(const char* content);
  Result updateVisit(const int32_t id, const char* content);
  Result getVisit(ConstBuffer& buffer, const int32_t id);
};

typedef std::shared_ptr<Storage> StoragePtr;

} // namespace db
#endif // _STORAGE_H_