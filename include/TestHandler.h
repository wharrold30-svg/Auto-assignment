#ifndef AA_SERVER_REQUEST_TEST_HANDLER_H
#define AA_SERVER_REQUEST_TEST_HANDLER_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Date.h"
#include "OpenTest.h"
#include "RequestHandler.h"
#include "Test.h"
#include "sql/test.h"

namespace AA::Server::Request {

class TestHandler final : public Handler {
 public:
  using Handler::Handler;

 protected:
  std::string_view entityName() const override { return "test"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& conn) override {
    AA::Data::test repo{conn};

    // Base DataBase CRUD operations
    if (action == "create" || action == "save") {
      return handleCreate(repo, payload);
    }
    if (action == "read" || action == "get") {
      return handleRead(repo, payload);
    }
    if (action == "update") {
      return handleUpdate(repo, payload);
    }
    if (action == "delete" || action == "remove") {
      return handleDelete(repo, payload);
    }

    // Test & question operations
    if (action == "save_with_questions" || action == "create_with_questions") {
      return handleSaveWithQuestions(repo, payload);
    }
    if (action == "load_with_questions" || action == "get_with_questions") {
      return handleLoadWithQuestions(repo, payload);
    }
    if (action == "load_paginated" || action == "list_paginated") {
      return handleLoadPaginated(repo, payload);
    }
    if (action == "save_questions") {
      return handleSaveQuestions(repo, payload);
    }
    if (action == "load_questions" || action == "get_questions") {
      return handleLoadQuestions(repo, payload);
    }
    if (action == "remove_questions" || action == "delete_questions") {
      return handleRemoveQuestions(repo, payload);
    }

    // Open test operations
    if (action == "save_open_test" || action == "create_open_test") {
      return handleSaveOpenTest(repo, payload);
    }
    if (action == "load_open_tests_by_date_range" ||
        action == "get_open_tests_by_date_range") {
      return handleLoadOpenTestsByDateRange(repo, payload);
    }

    return error(action, "Unsupported test action: " + action);
  }

 private:
  static constexpr int kMaxPageSize = 200;

  static AA::Test extractTest(const nlohmann::json& payload) {
    if (payload.contains("test") && payload["test"].is_object()) {
      return payload.at("test").get<AA::Test>();
    }
    return payload.get<AA::Test>();
  }

  static AA::OpenTest extractOpenTest(const nlohmann::json& payload) {
    if (payload.contains("open_test") && payload["open_test"].is_object()) {
      return payload.at("open_test").get<AA::OpenTest>();
    }
    return payload.get<AA::OpenTest>();
  }

  static TestVec extractQuestions(const nlohmann::json& payload) {
    if (payload.contains("questions") && payload["questions"].is_array()) {
      return payload.at("questions").get<TestVec>();
    }
    return payload.get<TestVec>();
  }

  static DT::Date parseDateField(const nlohmann::json& payload,
                                 const std::string& key) {
    if (!payload.contains(key) || payload[key].is_null()) {
      throw std::invalid_argument("Missing required date field: " + key);
    }
    if (payload[key].is_object()) {
      return payload.at(key).get<DT::Date>();
    }
    return DT::Date(requireField<std::string>(payload, key));
  }

  static std::string resolveIdColumn(const nlohmann::json& payload) {
    if (payload.contains("id_column") && !payload["id_column"].is_null()) {
      const std::string col = requireField<std::string>(payload, "id_column");
      if (col != "test_id") {
        throw std::invalid_argument("Unsupported id_column: " + col);
      }
      return col;
    }
    return "test_id";
  }

  static std::string resolveIdValue(const nlohmann::json& payload,
                                    const std::string& idColumn) {
    if (payload.contains(idColumn) && !payload[idColumn].is_null()) {
      return requireField<std::string>(payload, idColumn);
    }
    if (payload.contains("test") && payload["test"].is_object()) {
      const auto& nested = payload.at("test");
      if (nested.contains(idColumn) && !nested[idColumn].is_null()) {
        return requireField<std::string>(nested, idColumn);
      }
    }
    return requireField<std::string>(payload, "test_id");
  }

  // --- Base DataBase CRUD ---

  nlohmann::json handleCreate(AA::Data::test& repo,
                              const nlohmann::json& payload) const {
    const AA::Test testObj = extractTest(payload);
    if (!repo.save(testObj)) {
      return error("create", "Failed to save test record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test"] = testObj;
    return ok("create", std::move(result));
  }

  nlohmann::json handleRead(AA::Data::test& repo,
                            const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Test testObj{};
    if (!repo.read(idColumn, idValue, testObj)) {
      return error("read", "Test not found");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test"] = testObj;
    return ok("read", std::move(result));
  }

  nlohmann::json handleUpdate(AA::Data::test& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Test existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("update", "Test not found");
    }

    const AA::Test updated = extractTest(payload);
    if (!repo.update(updated, idColumn, idValue)) {
      return error("update", "Failed to update test record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test"] = updated;
    return ok("update", std::move(result));
  }

  nlohmann::json handleDelete(AA::Data::test& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    if (!repo.remove(idColumn, idValue)) {
      return error("delete", "Test not found or could not be deleted");
    }
    return ok("delete", {{"deleted", true}, {idColumn, idValue}});
  }

  // --- Tests & Questions ---

  nlohmann::json handleSaveWithQuestions(AA::Data::test& repo,
                                         const nlohmann::json& payload) const {
    const AA::Test testObj = extractTest(payload);
    if (!repo.saveTestWithQuestions(testObj)) {
      return error("save_with_questions", "Failed to save test with questions");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test"] = testObj;
    return ok("save_with_questions", std::move(result));
  }

  nlohmann::json handleLoadWithQuestions(AA::Data::test& repo,
                                         const nlohmann::json& payload) const {
    const std::string testId = requireField<std::string>(payload, "test_id");

    AA::Test testObj{};
    if (!repo.loadTestWithQuestions(testId, testObj)) {
      return error("load_with_questions", "Failed to load test with questions");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test"] = testObj;
    return ok("load_with_questions", std::move(result));
  }

  nlohmann::json handleLoadPaginated(AA::Data::test& repo,
                                     const nlohmann::json& payload) const {
    const int limit = requireField<int>(payload, "limit");
    const int offset =
        payload.contains("offset") && !payload["offset"].is_null()
            ? requireField<int>(payload, "offset")
            : 0;

    if (limit <= 0 || limit > kMaxPageSize || offset < 0) {
      return error("load_paginated", "Invalid pagination bounds");
    }

    std::vector<AA::Test> tests;
    if (!repo.loadTestsPaginated(limit, offset, tests)) {
      return error("load_paginated", "Failed to load paginated tests");
    }

    nlohmann::json result = nlohmann::json::object();
    result["limit"] = limit;
    result["offset"] = offset;
    result["tests"] = tests;
    return ok("load_paginated", std::move(result));
  }

  nlohmann::json handleSaveQuestions(AA::Data::test& repo,
                                     const nlohmann::json& payload) const {
    const std::string testId = requireField<std::string>(payload, "test_id");
    const TestVec questions = extractQuestions(payload);

    if (!repo.saveTestQuestions(testId, questions)) {
      return error("save_questions", "Failed to save test questions");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test_id"] = testId;
    result["questions"] = questions;
    return ok("save_questions", std::move(result));
  }

  nlohmann::json handleLoadQuestions(AA::Data::test& repo,
                                     const nlohmann::json& payload) const {
    const std::string testId = requireField<std::string>(payload, "test_id");

    TestVec questions;
    if (!repo.loadTestQuestions(testId, questions)) {
      return error("load_questions", "Failed to load test questions");
    }

    nlohmann::json result = nlohmann::json::object();
    result["test_id"] = testId;
    result["questions"] = questions;
    return ok("load_questions", std::move(result));
  }

  nlohmann::json handleRemoveQuestions(AA::Data::test& repo,
                                       const nlohmann::json& payload) const {
    const std::string testId = requireField<std::string>(payload, "test_id");

    if (!repo.removeTestQuestions(testId)) {
      return error("remove_questions", "Failed to remove questions for test");
    }

    return ok("remove_questions", {{"test_id", testId}, {"removed", true}});
  }

  // --- Open Tests ---

  nlohmann::json handleSaveOpenTest(AA::Data::test& repo,
                                    const nlohmann::json& payload) const {
    const AA::OpenTest openTest = extractOpenTest(payload);
    if (!repo.saveOpenTest(openTest)) {
      return error("save_open_test", "Failed to save open test");
    }

    nlohmann::json result = nlohmann::json::object();
    result["open_test"] = openTest;
    return ok("save_open_test", std::move(result));
  }

  nlohmann::json handleLoadOpenTestsByDateRange(
      AA::Data::test& repo, const nlohmann::json& payload) const {
    const DT::Date startDate = parseDateField(payload, "start_date");
    const DT::Date endDate = parseDateField(payload, "end_date");

    std::vector<AA::OpenTest> openTests;
    if (!repo.loadOpenTestsByDateRange(startDate, endDate, openTests)) {
      return error("load_open_tests_by_date_range",
                   "Failed to load open tests for date range");
    }

    nlohmann::json result = nlohmann::json::object();
    result["open_tests"] = openTests;
    return ok("load_open_tests_by_date_range", std::move(result));
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_TEST_HANDLER_H
