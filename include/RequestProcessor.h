#ifndef AA_SERVER_REQUEST_PROCESSOR_H
#define AA_SERVER_REQUEST_PROCESSOR_H

#include <exception>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "HandlerFactory.h"
#include "sql/ConnectionPool.h"

namespace AA::Server {

class RequestProcessor {
 public:
  explicit RequestProcessor(AA::Data::ConnectionPool& pool) noexcept
      : m_pool{pool} {}

  std::string operator()(std::string_view rawRequest) const {
    nlohmann::json response;

    try {
      nlohmann::json req = nlohmann::json::parse(rawRequest);
      auto handler =
          Request::HandlerFactory::newHandler(std::move(req), m_pool);
      response = handler->execute();
    } catch (const nlohmann::json::parse_error& e) {
      response = {{"status", "error"},
                  {"data", "unknown"},
                  {"action", "parse"},
                  {"message", std::string{"Invalid JSON: "} + e.what()}};
    } catch (const std::exception& e) {
      response = {{"status", "error"},
                  {"data", "unknown"},
                  {"action", "unknown"},
                  {"message", e.what()}};
    } catch (...) {
      response = {{"status", "error"},
                  {"data", "unknown"},
                  {"action", "unknown"},
                  {"message", "Unknown non-standard exception"}};
    }

    return response.dump();
  }

 private:
  AA::Data::ConnectionPool& m_pool;
};

}  // namespace AA::Server

#endif  // AA_SERVER_REQUEST_PROCESSOR_H
