#include <cstring>
#include <sstream>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <zip.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "Storage.h"

namespace db
{

static void safe_create_dir(const char *dir)
{
  if (mkdir(dir, 0755) < 0) {
    if (errno != EEXIST) {
      perror(dir);
      exit(1);
    }
  }
}

Result Storage::load(const std::string& path)
{
  char buf[1024];

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
    LOG(stderr, "==================\n");
    int len = strlen(stat.name);
    LOG(stderr, "Name: [%s], ", stat.name);
    LOG(stderr, "Size: [%lu], ", stat.size);
    LOG(stderr, "mtime: [%u]\n", (unsigned int)stat.mtime);
    if (stat.name[len - 1] == '/' ||
        stat.name[len - 1] == '\\') {
      safe_create_dir(stat.name);
      continue ;
    }

    struct zip_file* zf = zip_fopen_index(archive, i, 0);
    if (!zf) {
      continue;
    }

    {
      std::ostringstream ss;
      uint64_t sum = 0;
      while (sum != stat.size) {
        
        len = zip_fread(zf, buf, sizeof(buf));
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
        if (0 == strncmp(stat.name, "users_", 6)) {
          if (document.HasMember("users")) {
            const Value& users = document["users"];
            for (auto it = users.Begin(), end = users.End(); it != end; ++it) {
              // LOG(stderr, "%s\n", (*it)["first_name"].GetString());
            }
          }
        }
      }
    }
  }

  return Result::SUCCESS;
}

} // namespace db