#ifndef AA_HANDLER_FACTORY_H
#define AA_HANDLER_FACTORY_H

#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>

#include "GradebookHandler.h"
#include "RequestHandler.h"
#include "SchoolHandler.h"
#include "StudentHandler.h"
#include "TeacherHandler.h"
#include "TestHandler.h"
#include "sql/ConnectionPool.h"

namespace AA::Server::Request {

class HandlerFactory {
 public:
  static std::unique_ptr<Handler> newHandler(nlohmann::json request,
                                             AA::Data::ConnectionPool& pool) {
    if (!request.is_object()) {
      throw std::invalid_argument("Request must be a JSON object");
    }
    if (!request.contains("data") || !request["data"].is_string()) {
      throw std::invalid_argument("Missing or invalid 'data' field");
    }
    if (!request.contains("action") || !request["action"].is_string()) {
      throw std::invalid_argument("Missing or invalid 'action' field");
    }

    const std::string entity = request["data"].get<std::string>();

    if (entity == "test") {
      return std::make_unique<TestHandler>(std::move(request), pool);
    }
    if (entity == "teacher") {
      return std::make_unique<TeacherHandler>(std::move(request), pool);
    }
    if (entity == "student") {
      return std::make_unique<StudentHandler>(std::move(request), pool);
    }
    if (entity == "school") {
      return std::make_unique<SchoolHandler>(std::move(request), pool);
    }
    if (entity == "gradebook") {
      return std::make_unique<GradebookHandler>(std::move(request), pool);
    }

    throw std::runtime_error{"Invalid handler type: " + entity};
  }
};  // class HandlerFactory

}  // namespace AA::Server::Request

#endif  // AA_HANDLER_FACTORY_H
