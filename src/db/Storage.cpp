#include <cstring>
#include <unordered_set>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "Storage.h"

#include "UserHandler.h"
#include "LocationHandler.h"
#include "VisitHandler.h"

namespace db
{
struct tm Storage::time_;

std::unordered_map<in_place_string, UserHandler::State> UserHandler::strToState =
{
  { "id" ,          UserHandler::State::ID},
  { "email" ,       UserHandler::State::EMAIL},
  { "first_name" ,  UserHandler::State::FIRST_NAME},
  { "last_name" ,   UserHandler::State::LAST_NAME},
  { "birth_date" ,  UserHandler::State::BIRTH_DATE},
  { "gender" ,      UserHandler::State::GENDER},
};

std::unordered_map<in_place_string, LocationHandler::State> LocationHandler::strToState =
{
  { "id" ,          LocationHandler::State::ID},
  { "place" ,       LocationHandler::State::PLACE},
  { "country" ,     LocationHandler::State::COUNTRY},
  { "city" ,        LocationHandler::State::CITY},
  { "distance" ,    LocationHandler::State::DISTANCE},
};

std::unordered_map<in_place_string, VisitHandler::State> VisitHandler::strToState =
{
  { "id" ,          VisitHandler::State::ID},
  { "location" ,    VisitHandler::State::LOCATION},
  { "user" ,        VisitHandler::State::USER},
  { "visited_at" ,  VisitHandler::State::VISITED_AT},
  { "mark" ,        VisitHandler::State::MARK},
};

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
        std::forward_as_tuple(id, jsonVal));
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
        std::forward_as_tuple(id, jsonVal));
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
        std::forward_as_tuple(id, locationId, userId, location, user, jsonVal));
      Visit* visit = &res.first->second;

      user->visits_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(visit));

      location->visits_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(visit));
    });

  LOG_CRITICAL(stderr, "All information was loaded from files\n");

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

Result Storage::loadTime(const std::string& path)
{
  std::ifstream fin(path.c_str());
  time_t currentTime;
  fin >> currentTime;
  LOG(stderr, "Timestame loaded from file: %d\n", static_cast<int32_t>(currentTime));
  time_ = *gmtime(&currentTime);
  return Result::SUCCESS;
}

struct tm Storage::getTime()
{
  return time_;
}

Result Storage::addUser(const char* content)
{
  using namespace rapidjson;

  START_PROFILER("addUser");

  User user;
  UserHandler handler(user);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return Result::FAILED;
  }

  if (handler.filledFields_ < 6) {
    return Result::FAILED;
  }

  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, true);
  users_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(handler.id_),
    std::forward_as_tuple(std::move(user)));

  return Result::SUCCESS;
}

Result Storage::updateUser(const int32_t id, const char* content)
{
  using namespace rapidjson;
  START_PROFILER("updateUser");
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  tbb::spin_rw_mutex::scoped_lock userLock(it->second.guard_, true);
  l.release();
  User tmpUser(it->second);
  UserHandler handler(tmpUser);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return Result::FAILED;
  }

  if (0 == handler.filledFields_) {
    return Result::FAILED;
  }

  it->second = std::move(tmpUser);

  return Result::SUCCESS;
}

Result Storage::addLocation(const char* content)
{
  using namespace rapidjson;

  START_PROFILER("addLocation");
  Location location;
  LocationHandler handler(location);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return Result::FAILED;
  }

  if (handler.filledFields_ < 5) {
    return Result::FAILED;
  }

  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, true);
  locations_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(handler.id_),
    std::forward_as_tuple(std::move(location)));

  return Result::SUCCESS;
}

Result Storage::updateLocation(const int32_t id, const char* content)
{
  START_PROFILER("updateLocation");

  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  tbb::spin_rw_mutex::scoped_lock locationLock(it->second.guard_, true);
  l.release();

  Location tmpLocation(it->second);
  LocationHandler handler(tmpLocation);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return Result::FAILED;
  }

  if (0 == handler.filledFields_) {
    return Result::FAILED;
  }

  it->second = std::move(tmpLocation);

  return Result::SUCCESS;
}

Result Storage::addVisit(const char* content)
{
  using namespace rapidjson;
  START_PROFILER("addVisit");
  Visit visit;
  VisitHandler handler(visit, usersGuard_, users_, locationsGuard_, locations_);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return handler.result_;
  }

  if (handler.filledFields_ < 5) {
    return Result::FAILED;
  }

  tbb::spin_rw_mutex::scoped_lock visitsLock(visitsGuard_, true);
  auto res = visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(handler.id_),
    std::forward_as_tuple(std::move(visit)));
  Visit* visitPtr = &res.first->second;

  visit.user_->visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(handler.id_),
    std::forward_as_tuple(visitPtr));

  visit.location_->visits_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(handler.id_),
    std::forward_as_tuple(visitPtr));

  return Result::SUCCESS;
}

Result Storage::updateVisit(const int32_t id, const char* content)
{
  using namespace rapidjson;
  START_PROFILER("updateVisit");
  tbb::spin_rw_mutex::scoped_lock l(visitsGuard_, true);
  auto it = visits_.find(id);
  if (visits_.end() == it) {
    return Result::NOT_FOUND;
  }
  tbb::spin_rw_mutex::scoped_lock visitLock(it->second.guard_, true);
  l.release();

  Visit tmpVisit(it->second);
  VisitHandler handler(tmpVisit, usersGuard_, users_, locationsGuard_, locations_);
  Reader reader;
  StringStream ss(content);
  if (!reader.Parse(ss, handler)) {
    return handler.result_;
  }

  if (0 == handler.filledFields_) {
    return Result::FAILED;
  }

  if (tmpVisit.user_ && it->second.user_ != tmpVisit.user_) {
    it->second.user_->visits_.erase(id);
    it->second.user_ = tmpVisit.user_;
    it->second.user_->visits_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(&it->second));
  }

  if (tmpVisit.location_ && it->second.location_ != tmpVisit.location_) {
    it->second.location_->visits_.erase(id);
    it->second.location_ = tmpVisit.location_;
    it->second.location_->visits_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(&it->second));
  }

  it->second = std::move(tmpVisit);

  return Result::SUCCESS;
}

Result Storage::getUser(Buffer& buffer, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  it->second.getJson(buffer, id);
  return Result::SUCCESS;
}

Result Storage::getLocation(Buffer& buffer, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  it->second.getJson(buffer, id);
  return Result::SUCCESS;
}

Result Storage::getVisit(Buffer& buffer, const int32_t id)
{
  tbb::spin_rw_mutex::scoped_lock l(visitsGuard_, false);
  auto it = visits_.find(id);
  if (visits_.end() == it) {
    return Result::NOT_FOUND;
  }
  it->second.getJson(buffer, id);
  return Result::SUCCESS;
}

Result Storage::getUserVisits(Buffer& buffer, const int32_t id, char* params, const int32_t paramsSize)
{
  tbb::spin_rw_mutex::scoped_lock l(usersGuard_, false);
  auto it = users_.find(id);
  if (users_.end() == it) {
    return Result::NOT_FOUND;
  }
  return it->second.getJsonVisits(buffer, params, paramsSize);
}

Result Storage::getLocationAvgScore(Buffer& buffer, const int32_t id, char* params, const int32_t paramsSize)
{
  tbb::spin_rw_mutex::scoped_lock l(locationsGuard_, false);
  auto it = locations_.find(id);
  if (locations_.end() == it) {
    return Result::NOT_FOUND;
  }
  return it->second.getJsonAvgScore(buffer, params, paramsSize);
}

} // namespace db