#ifndef _VISIT_HANDLER_H_
#define _VISIT_HANDLER_H_

#include <cstdint>
#include <unordered_map>
#include <string>

#include <tbb/spin_rw_mutex.h>

#include <rapidjson/reader.h>

#include "Visit.h"

namespace db
{

using namespace rapidjson;

struct VisitHandler {
  enum class State {
    ID = 0,
    LOCATION,
    USER,
    VISITED_AT,
    MARK,
  };
  static std::unordered_map<in_place_string, State> strToState;

  State state_;
  uint16_t filledFields_;
  int32_t id_;
  Visit& visit_;
  Result result_ = Result::FAILED;
  tbb::spin_rw_mutex& usersGuard_;
  std::unordered_map<int32_t, User>& users_;
  tbb::spin_rw_mutex::scoped_lock userLock_;
  tbb::spin_rw_mutex& locationsGuard_;
  std::unordered_map<int32_t, Location>& locations_;
  tbb::spin_rw_mutex::scoped_lock locationLock_;

  VisitHandler(
    Visit& visit,
    tbb::spin_rw_mutex& usersGuard,
    std::unordered_map<int32_t, User>& users,
    tbb::spin_rw_mutex& locationsGuard,
    std::unordered_map<int32_t, Location>& locations) :
    filledFields_(0),
    visit_(visit),
    usersGuard_(usersGuard),
    users_(users),
    locationsGuard_(locationsGuard),
    locations_(locations)
  {}

  bool Null() { return false; }
  bool Bool(bool b) { return false; }
  
  template<class T>
  bool Integer(T t)
  {
    switch (state_) {
      case State::ID:
        id_ = static_cast<int32_t>(t);
        break;
      case State::LOCATION:
      {
        visit_.location = static_cast<int32_t>(t);
        tbb::spin_rw_mutex::scoped_lock locationsLock(locationsGuard_, true);
        auto locationIt = locations_.find(visit_.location);
        if (locations_.end() == locationIt) {
          result_ = Result::NOT_FOUND;
          return false;
        }
        visit_.location_ = &locationIt->second;
        locationLock_.acquire(visit_.location_->guard_, true);
        locationsLock.release();
        break;
      }
      case State::USER:
      {
        visit_.user = static_cast<int32_t>(t);
        tbb::spin_rw_mutex::scoped_lock usersLock(usersGuard_, true);
        auto userIt = users_.find(visit_.user);
        if (users_.end() == userIt) {
          result_ = Result::NOT_FOUND;
          return false;
        }
        visit_.user_ = &userIt->second;
        userLock_.acquire(visit_.user_->guard_, true);
        usersLock.release();
        break;
      }
      case State::VISITED_AT:
        visit_.visited_at = static_cast<int32_t>(t);
        break;
      case State::MARK:
        visit_.mark = static_cast<uint16_t>(t);
        break;
    }
    ++ filledFields_;
    return true;
  }
  bool Int(int i) {return Integer(i);}
  bool Uint(unsigned u) {return Integer(u);}
  bool Int64(int64_t i) {return Integer(i);}
  bool Uint64(uint64_t u) {return Integer(u);}
  bool Double(double d) { return false; }
  bool RawNumber(const char* str, SizeType length, bool copy) { return false; }
  bool String(const char* str, SizeType length, bool copy) { return false; }
  bool StartObject() { return true; }
  bool Key(const char* str, SizeType length, bool copy) {
    auto it = strToState.find(in_place_string(str, length));
    if (strToState.end() == it) {
      return false;
    }
    state_ = it->second;
    return true;
  }
  bool EndObject(SizeType memberCount) { return true; }
  bool StartArray() { return true; }
  bool EndArray(SizeType elementCount) { return true; }
};
} // namespace db

#endif // _VISIT_HANDLER_H_