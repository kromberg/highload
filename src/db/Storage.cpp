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

  loadFiles(archive, stats[usersFilePrefix], "users",
    [&] (const rapidjson::Value& jsonVal) -> void {
      if (!jsonVal.HasMember("id")) {
        return ;
      }
      const int32_t id = jsonVal["id"].GetInt();
      users_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(jsonVal));
    });
  loadFiles(archive, stats[locationsFilePrefix], "locations",
    [&] (const rapidjson::Value& jsonVal) -> void {
      if (!jsonVal.HasMember("id")) {
        return ;
      }
      const int32_t id = jsonVal["id"].GetInt();
      locations_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(jsonVal));
    });
  loadFiles(archive, stats[visitsFilePrefix], "visits",
    [&] (const rapidjson::Value& jsonVal) -> void {
      if (!jsonVal.HasMember("id")) {
        return ;
      }
      const int32_t id = jsonVal["id"].GetInt();
      if (!jsonVal.HasMember("location")) {
        return ;
      }
      const int32_t locationId = jsonVal["location"].GetInt();
      if (!jsonVal.HasMember("user")) {
        return ;
      }
      const int32_t userId = jsonVal["user"].GetInt();

      auto userIt = users_.find(userId);
      if (users_.end() == userIt) {
        return ;
      }
      User* user = &userIt->second;

      auto locationIt = locations_.find(locationId);
      if (locations_.end() == locationIt) {
        return ;
      }
      Location* location = &locationIt->second;

      auto res = visits_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(locationId, userId, location, user, jsonVal));
      Visit* visit = &res.first->second;

      user->visits_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(visit));

      location->visits_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(user, visit));
    });

  return Result::SUCCESS;
}

void Storage::loadFiles(
  zip *archive,
  ZipStats& zipStats,
  const std::string& arrayName,
  const MapLoaderFunc& func)
{
  LOG(stderr, "Loading to '%s' from %zu file(s)\n", arrayName.c_str(), zipStats.size());
  for (auto& stat : zipStats) {
    LOG(stderr, "Loading from file %s\n", stat.name);

    struct zip_file* zf = zip_fopen(archive, stat.name, 0);
    if (!zf) {
      continue;
    }

    {
      size_t size = stat.size;
      size_t offset = 0;
      char* buf = new char[size + 1];
      while (offset != size) {
        int len = zip_fread(zf, buf + offset, size - offset);
        if (len < 0) {
          break;
        }
        offset += len;
      }
      buf[size] = '\0';
      zip_fclose(zf);

      {
        // parse json
        using namespace rapidjson;
        Document document;
        document.Parse(buf);
        if (!document.HasMember(arrayName.c_str())) {
          continue;
        }
        const Value& jsonArr = document[arrayName.c_str()];
        uint64_t loaded = 0;
        for (auto it = jsonArr.Begin(), end = jsonArr.End(); it != end; ++it) {
          try {
            func(*it);
            ++ loaded;
          } catch (...) {
            LOG(stderr, "Error occurred while parsing entry from '%s'\n", arrayName.c_str());
            continue;
          }
        }
        LOG(stderr, "Loaded %lu entries from %s file\n", loaded, stat.name);
      }

      delete[] buf;
    }
  }
}

Result Storage::addUser(const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id;
  {
    const Value& val = jsonVal["id"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    id = val.GetInt();
  }

  if (!jsonVal.HasMember("email")) {
    return Result::FAILED;
  }
  std::string email;
  {
    const Value& val = jsonVal["email"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    email = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("first_name")) {
    return Result::FAILED;
  }
  std::string first_name;
  {
    const Value& val = jsonVal["first_name"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    first_name = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("last_name")) {
    return Result::FAILED;
  }
  std::string last_name;
  {
    const Value& val = jsonVal["last_name"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    last_name = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("birth_date")) {
    return Result::FAILED;
  }
  int32_t birth_date;
  {
    const Value& val = jsonVal["birth_date"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    birth_date = val.GetInt();
  }

  if (!jsonVal.HasMember("gender")) {
    return Result::FAILED;
  }
  uint16_t gender;
  {
    const Value& val = jsonVal["gender"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    const char* genderStr = val.GetString();
    if (*genderStr == 'f') {
      gender = false;
    } else if (*genderStr == 'm') {
      gender = true;
    }
  }

  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, true);
  users_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(std::move(email), std::move(first_name), std::move(last_name), birth_date, gender));

  return Result::SUCCESS;
}

Result Storage::addLocation(const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id;
  {
    const Value& val = jsonVal["id"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    id = val.GetInt();
  }

  if (!jsonVal.HasMember("place")) {
    return Result::FAILED;
  }
  std::string place;
  {
    const Value& val = jsonVal["place"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    place = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("country")) {
    return Result::FAILED;
  }
  std::string country;
  {
    const Value& val = jsonVal["country"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    country = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("city")) {
    return Result::FAILED;
  }
  std::string city;
  {
    const Value& val = jsonVal["city"];
    if (!val.IsString()) {
      return Result::FAILED;
    }
    city = std::string(val.GetString());
  }

  if (!jsonVal.HasMember("distance")) {
    return Result::FAILED;
  }
  int32_t distance;
  {
    const Value& val = jsonVal["distance"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    distance = val.GetInt();
  }

  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, true);
  locations_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(std::move(place), std::move(country), std::move(city), distance));

  return Result::SUCCESS;
}

Result Storage::addVisit(const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;
  if (!jsonVal.HasMember("id")) {
    return Result::FAILED;
  }
  int32_t id;
  {
    const Value& val = jsonVal["id"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    id = val.GetInt();
  }

  if (!jsonVal.HasMember("location")) {
    return Result::FAILED;
  }
  int32_t locationId;
  {
    const Value& val = jsonVal["location"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    locationId = val.GetInt();
  }

  if (!jsonVal.HasMember("user")) {
    return Result::FAILED;
  }
  int32_t userId;
  {
    const Value& val = jsonVal["user"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    userId = val.GetInt();
  }

  if (!jsonVal.HasMember("visited_at")) {
    return Result::FAILED;
  }
  int32_t visited_at;
  {
    const Value& val = jsonVal["visited_at"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    visited_at = val.GetInt();
  }

  if (!jsonVal.HasMember("mark")) {
    return Result::FAILED;
  }
  int32_t mark;
  {
    const Value& val = jsonVal["mark"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    mark = val.GetInt();
  }

  tbb::spin_rw_mutex::scoped_lock usersLock(usersGuard_, false);
  auto userIt = users_.find(userId);
  if (users_.end() == userIt) {
    return Result::NOT_FOUND;
  }
  User* user = &userIt->second;

  tbb::spin_rw_mutex::scoped_lock locationsLock(locationsGuard_, false);
  auto locationIt = locations_.find(locationId);
  if (locations_.end() == locationIt) {
    return Result::NOT_FOUND;
  }
  Location* location = &locationIt->second;

  usersLock.upgrade_to_writer();
  locationsLock.upgrade_to_writer();
  tbb::spin_rw_mutex::scoped_lock visitsLock(visitsGuard_, true);
  auto res = visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(locationId, userId, visited_at, mark, location, user));
  Visit* visit = &res.first->second;

  user->visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(visit));

  location->visits_.emplace(
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
  if (!it->second.update(jsonVal)) {
    return Result::FAILED;
  }

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
  if (!it->second.update(jsonVal)) {
    return Result::FAILED;
  }

  return Result::SUCCESS;
}

Result Storage::updateVisit(const int32_t id, const rapidjson::Value& jsonVal)
{
  using namespace rapidjson;

  tbb::spin_rw_mutex::scoped_lock usersLock;
  int32_t userId = -1;
  User *user = nullptr;
  if (jsonVal.HasMember("user")) {
    const Value& val = jsonVal["user"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    userId = val.GetInt();
    usersLock.acquire(usersGuard_, false);
    auto userIt = users_.find(userId);
    if (users_.end() == userIt) {
      return Result::NOT_FOUND;
    }
    user = &userIt->second;
  }

  tbb::spin_rw_mutex::scoped_lock locationsLock;
  int32_t locationId = -1;
  Location *location = nullptr;
  if (jsonVal.HasMember("location")) {
    const Value& val = jsonVal["location"];
    if (!val.IsInt()) {
      return Result::FAILED;
    }
    locationId = val.GetInt();

    locationsLock.acquire(locationsGuard_, false);
    auto locationIt = locations_.find(locationId);
    if (locations_.end() == locationIt) {
      return Result::NOT_FOUND;
    }
    location = &locationIt->second;
  }

  if (user) {
    usersLock.upgrade_to_writer();
  }
  if (location) {
    locationsLock.upgrade_to_writer();
  }
  tbb::spin_rw_mutex::scoped_lock l(visitsGuard_, false);
  auto it = visits_.find(id);
  if (visits_.end() == it) {
    return Result::NOT_FOUND;
  }
  const int32_t prevLocation = it->second.location;
  const int32_t prevUser = it->second.user;
  l.upgrade_to_writer();
  if (!it->second.update(locationId, userId, jsonVal)) {
    return Result::FAILED;
  }

  if (user && prevUser != userId) {
    it->second.user_ = user;
    user->visits_[id] = &it->second;
  }

  if (location && prevLocation != locationId) {
    it->second.location_ = location;
    location->visits_[id] = std::make_pair(it->second.user_, &it->second);
  }

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

Result Storage::getUserVisits(std::string& resp, const int32_t id, const char* params, const int32_t paramsSize)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  return it->second.getJsonVisits(resp, params, paramsSize);
}

Result Storage::getLocationAvgScore(std::string& resp, const int32_t id, const char* params, const int32_t paramsSize)
{
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  return it->second.getJsonAvgScore(resp, params, paramsSize);
}

} // namespace db