#ifndef _USER_HANDLER_H_
#define _USER_HANDLER_H_

#include <cstdint>
#include <unordered_map>
#include <string>

#include <rapidjson/reader.h>

#include "User.h"

namespace db
{

using namespace rapidjson;

struct UserHandler {
  enum class State {
    ID = 0,
    EMAIL,
    FIRST_NAME,
    LAST_NAME,
    BIRTH_DATE,
    GENDER,
  };
  static std::unordered_map<std::string, State> strToState;
  State state_;
  uint16_t filledFields_;
  int32_t id_;
  User& user_;

  UserHandler(User& user) :
    filledFields_(0),
    user_(user)
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
      case State::BIRTH_DATE:
        user_.birth_date = static_cast<int32_t>(t);
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
      case State::EMAIL:
        user_.email = std::string(str, length);
        break;
      case State::FIRST_NAME:
        user_.first_name = std::string(str, length);
        break;
      case State::LAST_NAME:
        user_.last_name = std::string(str, length);
        break;
      case State::GENDER:
        user_.gender = *str;
        break;
      default:
        return false;
    }
    ++ filledFields_;
    return true;
  }
  bool StartObject() { return true; }
  bool Key(const char* str, SizeType length, bool copy) {
    auto it = strToState.find(std::string(str, length));
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

#endif // _USER_HANDLER_H_