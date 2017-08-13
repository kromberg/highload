#include <common/Types.h>

#include "StateMachine.h"

namespace http
{
using common::Result;
using emptyH=StateMachine::Handler;

StateMachine::Handler StateMachine::handlersMatrix_[size_t(Type::MAX)][size_t(Table::MAX)][size_t(Table::MAX)] =
{
  // NONE
  {
    // NONE USERS LOCATIONS VISITS AVG
    // NONE
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // USERS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // LOCATIONS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // VISITS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
  },
  // POST
  {
    // NONE USERS LOCATIONS VISITS AVG
    // NONE
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // USERS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // LOCATIONS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // VISITS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
  },
  // GET
  {
    // NONE USERS LOCATIONS VISITS AVG
    // NONE
    { emptyH(), StateMachine::getUser, emptyH(), emptyH(), emptyH(), },
    // USERS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // LOCATIONS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // VISITS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
  },
};


HTTPCode StateMachine::getUser(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getUser(resp, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  }

  return HTTPCode::OK;
}

StateMachine::Handler StateMachine::getHandler(const Request& req)
{
  return handlersMatrix_[size_t(req.type_)][size_t(req.table1_)][size_t(req.table2_)];
}

} // namespace http
