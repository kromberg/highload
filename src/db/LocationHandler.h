#ifndef _LOCATION_HANDLER_H_
#define _LOCATION_HANDLER_H_

#include <cstdint>
#include <unordered_map>
#include <string>

#include <rapidjson/reader.h>

#include "Location.h"

namespace db
{

using namespace rapidjson;

struct LocationHandler {
  enum class State {
    ID = 0,
    PLACE,
    COUNTRY,
    CITY,
    DISTANCE,
  };
  static std::unordered_map<in_place_string, State> strToState;
  State state_;
  uint16_t filledFields_;
  int32_t id_;
  Location& location_;

  LocationHandler(Location& location) :
    filledFields_(0),
    location_(location)
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
      case State::DISTANCE:
        location_.distance = static_cast<int32_t>(t);
        break;
      default:
        return false;
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
  bool String(const char* str, SizeType length, bool copy)
  { 
    switch (state_) {
      case State::PLACE:
        location_.place = std::string(str, length);
        break;
      case State::COUNTRY:
        location_.country = std::string(str, length);
        break;
      case State::CITY:
        location_.city = std::string(str, length);
        break;
      default:
        return false;
    }
    ++ filledFields_;
    return true;
  }
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

#endif // _LOCATION_HANDLER_H_