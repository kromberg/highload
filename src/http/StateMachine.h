#ifndef _HTTP_STATE_MACHINE_H_
#define _HTTP_STATE_MACHINE_H_

#include <functional>

#include <tcp/Socket.h>
#include <db/Storage.h>

#include "Types.h"

namespace http
{


class StateMachine
{
public:
  typedef std::function<HTTPCode(std::string&, db::Storage&, Request&)> Handler;

private:
  static Handler handlersMatrix_[size_t(Type::MAX)][size_t(Table::MAX)][size_t(Table::MAX)];

  static HTTPCode addOrUpdateUser(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode addOrUpdateLocation(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode addOrUpdateVisit(std::string& resp, db::Storage& storage, Request& req);

  static HTTPCode addUser(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode addLocation(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode addVisit(std::string& resp, db::Storage& storage, Request& req);

  static HTTPCode updateUser(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode updateLocation(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode updateVisit(std::string& resp, db::Storage& storage, Request& req);

  static HTTPCode getUser(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode getLocation(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode getVisit(std::string& resp, db::Storage& storage, Request& req);

  static HTTPCode getUserVisits(std::string& resp, db::Storage& storage, Request& req);
  static HTTPCode getLocationScore(std::string& resp, db::Storage& storage, Request& req);
public:
  

  static Handler getHandler(const Request& req);
};

} // namespace http
#endif // _HTTP_STATE_MACHINE_H_