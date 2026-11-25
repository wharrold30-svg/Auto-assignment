#ifndef AA_REQUEST_HANDLER_H
#define AA_REQUEST_HANDLER_H

#include <mysql++/mysql++.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "sql/ConnectionPool.h"

namespace AA::Server::Request {

class Handler {
 protected:
  nlohmann::json m_request;
  AA::Data::ConnectionPool& m_pool;

  virtual std::string_view entityName() const = 0;

  virtual nlohmann::json executeImpl(const std::string& action,
                                     const nlohmann::json& payload,
                                     mysqlpp::Connection& conn) = 0;

  static nlohmann::json makeOk(
      std::string_view entity, std::string_view action,
      nlohmann::json result = nlohmann::json::object()) {
    return {{"status", "ok"},
            {"data", entity},
            {"action", action},
            {"result", std::move(result)}};
  }

  static nlohmann::json makeError(std::string_view entity,
                                  std::string_view action,
                                  std::string_view message) {
    return {{"status", "error"},
            {"data", entity},
            {"action", action},
            {"message", message}};
  }

  nlohmann::json ok(std::string_view action,
                    nlohmann::json result = nlohmann::json::object()) const {
    return makeOk(entityName(), action, std::move(result));
  }

  nlohmann::json error(std::string_view action,
                       std::string_view message) const {
    return makeError(entityName(), action, message);
  }

  template <typename T>
  static T requireField(const nlohmann::json& payload, const std::string& key) {
    if (!payload.is_object() || !payload.contains(key)) {
      throw std::invalid_argument("Missing required field: " + key);
    }
    const auto& val = payload.at(key);
    if (val.is_null()) {
      throw std::invalid_argument("Required field is null: " + key);
    }
    return val.get<T>();
  }

 public:
  Handler(nlohmann::json request, AA::Data::ConnectionPool& pool)
      : m_request(std::move(request)), m_pool{pool} {}

  virtual ~Handler() = default;

  nlohmann::json execute() {
    const std::string_view entity = entityName();
    std::string action = "unknown";

    try {
      if (!m_request.is_object()) {
        return makeError(entity, action, "Request must be a JSON object");
      }

      if (!m_request.contains("action") || !m_request["action"].is_string()) {
        return makeError(entity, action,
                         "Missing or non-string 'action' field");
      }
      action = m_request["action"].get<std::string>();

      static const nlohmann::json kEmptyObj = nlohmann::json::object();
      const nlohmann::json& payload =
          (m_request.contains("payload") && !m_request["payload"].is_null())
              ? m_request["payload"]
              : kEmptyObj;

      if (!payload.is_object()) {
        return makeError(entity, action, "'payload' must be a JSON object");
      }

      mysqlpp::ScopedConnection scopedConn(m_pool, true);
      if (!scopedConn) {
        return makeError(entity, action,
                         "Failed to acquire database connection from pool");
      }

      return executeImpl(action, payload, *scopedConn);
    } catch (const nlohmann::json::exception& e) {
      return makeError(entity, action,
                       std::string("JSON format/type error: ") + e.what());
    } catch (const mysqlpp::Exception& e) {
      return makeError(entity, action,
                       std::string("Database error: ") + e.what());
    } catch (const std::exception& e) {
      return makeError(entity, action, e.what());
    }
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_HANDLER_H
