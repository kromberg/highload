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
  typedef std::function<HTTPCode(Response&, db::Storage&, Request&)> Handler;

private:
  static Handler handlersMatrix_[size_t(Type::MAX)][size_t(Table::MAX)][size_t(Table::MAX)];

  static HTTPCode addOrUpdateUser(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode addOrUpdateLocation(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode addOrUpdateVisit(Response& resp, db::Storage& storage, Request& req);

  static HTTPCode addUser(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode addLocation(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode addVisit(Response& resp, db::Storage& storage, Request& req);

  static HTTPCode updateUser(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode updateLocation(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode updateVisit(Response& resp, db::Storage& storage, Request& req);

  static HTTPCode getUser(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode getLocation(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode getVisit(Response& resp, db::Storage& storage, Request& req);

  static HTTPCode getUserVisits(Response& resp, db::Storage& storage, Request& req);
  static HTTPCode getLocationAvgScore(Response& resp, db::Storage& storage, Request& req);
public:
  

  static Handler getHandler(const Request& req);
};

} // namespace http
#endif // _HTTP_STATE_MACHINE_H_