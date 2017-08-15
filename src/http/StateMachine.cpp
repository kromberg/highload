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
    { emptyH(), StateMachine::addOrUpdateUser, StateMachine::addOrUpdateLocation, StateMachine::addOrUpdateVisit, emptyH(), },
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
    { emptyH(), StateMachine::getUser, StateMachine::getLocation, StateMachine::getVisit, emptyH(), },
    // USERS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // LOCATIONS
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // VISITS
    { emptyH(), StateMachine::getUserVisits, emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), StateMachine::getLocationScore, emptyH(), emptyH(), emptyH(), },
  },
};

HTTPCode StateMachine::addOrUpdateUser(std::string& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addUser(resp, storage, req);
  } else {
    return updateUser(resp, storage, req);
  }
}

HTTPCode StateMachine::addOrUpdateLocation(std::string& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addLocation(resp, storage, req);
  } else {
    return updateLocation(resp, storage, req);
  }
}

HTTPCode StateMachine::addOrUpdateVisit(std::string& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addVisit(resp, storage, req);
  } else {
    return updateVisit(resp, storage, req);
  }
}

HTTPCode StateMachine::addUser(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addUser(req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::addLocation(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addLocation(req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::addVisit(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addVisit(req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateUser(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateUser(req.id_, req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateLocation(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateLocation(req.id_, req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateVisit(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateVisit(req.id_, req.json_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getUser(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getUser(resp, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getLocation(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getLocation(resp, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getVisit(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getVisit(resp, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getUserVisits(std::string& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getUserVisits(resp, req.id_, req.params_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }
  return HTTPCode::OK;
}

HTTPCode StateMachine::getLocationScore(std::string& resp, db::Storage& storage, Request& req)
{
  return HTTPCode::NOT_FOUND;
}

StateMachine::Handler StateMachine::getHandler(const Request& req)
{
  return handlersMatrix_[size_t(req.type_)][size_t(req.table1_)][size_t(req.table2_)];
}

} // namespace http
