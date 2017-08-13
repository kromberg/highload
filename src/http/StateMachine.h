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

  static HTTPCode getUser(std::string& resp, db::Storage& storage, Request& req);
public:
  

  static Handler getHandler(const Request& req);
};

} // namespace http
#endif // _HTTP_STATE_MACHINE_H_