#include <cstring>
#include <unordered_set>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "Storage.h"

namespace db
{

Result Storage::load(const std::string& path)
{
  static std::string usersFilePrefix("users_");
  static std::string locationsFilePrefix("locations_");
  static std::string visitsFilePrefix("visits_");

  std::unordered_map<std::string, ZipStats> stats = 
  {
    {usersFilePrefix,     ZipStats() },
    {locationsFilePrefix, ZipStats() },
    {visitsFilePrefix,    ZipStats() },
  };

  int err = 0;
  zip *archive = zip_open(path.c_str(), 0, &err);
  if (!archive) {
    return Result::FAILED;
  }

  for (int64_t i = 0, entries = zip_get_num_entries(archive, 0); i < entries; ++i) {
    struct zip_stat stat;
    int res = zip_stat_index(archive, i, 0, &stat);
    if (res < 0) {
      continue;
    }
    std::string filename(stat.name);
    for (auto& statEntry : stats) {
      if (0 == filename.compare(0, statEntry.first.size(), statEntry.first)) {
        statEntry.second.emplace_back(std::move(stat));
        break;
      }
    }
  }

  loadFiles(users_, archive, stats[usersFilePrefix], "users");
  loadFiles(locations_, archive, stats[locationsFilePrefix], "locations");
  loadFiles(visits_, archive, stats[visitsFilePrefix], "visits");

  for (auto& visit : visits_) {
    auto userIt = users_.find(visit.second.user);
    if (users_.end() == userIt) {
      continue;
    }
    userIt->second.visits_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(visit.first),
      std::forward_as_tuple(&(visit.second)));

    auto locationIt = locations_.find(visit.second.location);
    if (locations_.end() == locationIt) {
      return Result::SUCCESS;
    }
    locationIt->second.visits_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(visit.first),
      std::forward_as_tuple(&userIt->second, &visit.second));
  }

  return Result::SUCCESS;
}

Result Storage::addUser(const rapidjson::Value& jsonVal)
{
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id = jsonVal["id"].GetInt();
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, true);
  users_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(jsonVal));

  return Result::SUCCESS;
}

Result Storage::addLocation(const rapidjson::Value& jsonVal)
{
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id = jsonVal["id"].GetInt();
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, true);
  locations_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(jsonVal));

  return Result::SUCCESS;
}

Result Storage::addVisit(const rapidjson::Value& jsonVal)
{
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id = jsonVal["id"].GetInt();
  tbb::spin_rw_mutex::scoped_lock visitsLock(visitsGuard_, true);
  auto res = visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(jsonVal));

  Visit* visit = &res.first->second;
  auto userId = visit->user;
  auto locationId = visit->location;
  visitsLock.release();

  tbb::spin_rw_mutex::scoped_lock usersLock(usersGuard_, false);
  auto userIt = users_.find(userId);
  if (users_.end() == userIt) {
    return Result::SUCCESS;
  }
  userIt->second.visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(visit));
  User* user = &userIt->second;
  usersLock.release();

  tbb::spin_rw_mutex::scoped_lock locationsLock(locationsGuard_, false);
  auto locationIt = locations_.find(locationId);
  if (locations_.end() == locationIt) {
    return Result::SUCCESS;
  }
  locationIt->second.visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(user, visit));

  return Result::SUCCESS;
}

Result Storage::updateUser(const int32_t id, const rapidjson::Value& jsonVal)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  l.upgrade_to_writer();
  it->second.update(jsonVal);

  return Result::SUCCESS;
}

Result Storage::updateLocation(const int32_t id, const rapidjson::Value& jsonVal)
{
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  l.upgrade_to_writer();
  it->second.update(jsonVal);

  return Result::SUCCESS;
}

Result Storage::updateVisit(const int32_t id, const rapidjson::Value& jsonVal)
{
  tbb::spin_rw_mutex::scoped_lock l(visitsGuard_, false);
  auto it = visits_.find(id);
  if (visits_.end() == it) {
    return Result::NOT_FOUND;
  }
  l.upgrade_to_writer();
  it->second.update(jsonVal);

  return Result::SUCCESS;
}

Result Storage::getUser(std::string& resp, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  resp = std::move(it->second.getJson(id));
  return Result::SUCCESS;
}

Result Storage::getLocation(std::string& resp, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  resp = std::move(it->second.getJson(id));
  return Result::SUCCESS;
}

Result Storage::getVisit(std::string& resp, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(visitsGuard_, false);
  auto it = visits_.find(id);
  if (visits_.end() == it) {
    return Result::NOT_FOUND;
  }
  resp = std::move(it->second.getJson(id));
  return Result::SUCCESS;
}

Result Storage::getUserVisits(std::string& resp, const int32_t id, std::string& params)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  resp = std::move(it->second.getJsonVisits(params));
  return Result::SUCCESS;
}

} // namespace db