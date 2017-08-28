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
    { StateMachine::addOrUpdateUser, emptyH(), emptyH(), emptyH(), emptyH(), },
    // LOCATIONS
    { StateMachine::addOrUpdateLocation, emptyH(), emptyH(), emptyH(), emptyH(), },
    // VISITS
    { StateMachine::addOrUpdateVisit, emptyH(), emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
  },
  // GET
  {
    // NONE USERS LOCATIONS VISITS AVG
    // NONE
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
    // USERS
    { StateMachine::getUser, emptyH(), emptyH(), StateMachine::getUserVisits, emptyH(), },
    // LOCATIONS
    { StateMachine::getLocation, emptyH(), emptyH(), emptyH(), StateMachine::getLocationAvgScore, },
    // VISITS
    { StateMachine::getVisit, emptyH(), emptyH(), emptyH(), emptyH(), },
    // AVG
    { emptyH(), emptyH(), emptyH(), emptyH(), emptyH(), },
  },
};

HTTPCode StateMachine::addOrUpdateUser(Response& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addUser(resp, storage, req);
  } else {
    return updateUser(resp, storage, req);
  }
}

HTTPCode StateMachine::addOrUpdateLocation(Response& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addLocation(resp, storage, req);
  } else {
    return updateLocation(resp, storage, req);
  }
}

HTTPCode StateMachine::addOrUpdateVisit(Response& resp, db::Storage& storage, Request& req)
{
  if (-1 == req.id_) {
    return addVisit(resp, storage, req);
  } else {
    return updateVisit(resp, storage, req);
  }
}

HTTPCode StateMachine::addUser(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addUser(req.content_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::addLocation(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addLocation(req.content_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::addVisit(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.addVisit(req.content_);
  if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateUser(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateUser(req.id_, req.content_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateLocation(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateLocation(req.id_, req.content_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::updateVisit(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.updateVisit(req.id_, req.content_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getUser(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getUser(resp.buffer, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getLocation(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getLocation(resp.buffer, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getVisit(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getVisit(resp.buffer, req.id_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }

  return HTTPCode::OK;
}

HTTPCode StateMachine::getUserVisits(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getUserVisits(resp.buffer, req.id_, req.params_, req.paramsSize_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }
  return HTTPCode::OK;
}

HTTPCode StateMachine::getLocationAvgScore(Response& resp, db::Storage& storage, Request& req)
{
  Result res = storage.getLocationAvgScore(resp.buffer, req.id_, req.params_, req.paramsSize_);
  if (Result::NOT_FOUND == res) {
    return HTTPCode::NOT_FOUND;
  } else if (Result::SUCCESS != res) {
    return HTTPCode::BAD_REQ;
  }
  return HTTPCode::OK;
}

StateMachine::Handler StateMachine::getHandler(const Request& req)
{
  return handlersMatrix_[size_t(req.type_)][size_t(req.table1_)][size_t(req.table2_)];
}

} // namespace http
