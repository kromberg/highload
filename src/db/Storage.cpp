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


} // namespace db