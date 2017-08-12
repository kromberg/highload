#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <string>

#include <common/Types.h>

namespace db
{
using common::Result;
class Storage
{
private:
public:
  Result load(const std::string& path);
};
} // namespace db
#endif // _STORAGE_H_