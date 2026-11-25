#ifndef AA_STUDENT_HANDLER_H
#define AA_STUDENT_HANDLER_H

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Date.h"
#include "GradeRecord.h"
#include "OpenTest.h"
#include "RequestHandler.h"
#include "Student.h"
#include "sql/student.h"

namespace AA::Server::Request {

class StudentHandler final : public Handler {
 public:
  using Handler::Handler;

 protected:
  std::string_view entityName() const override { return "student"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& conn) override {
    AA::Data::student repo{conn};

    // Base DataBase CRUD operations
    if (action == "create" || action == "save") {
      return handleCreate(repo, payload);
    }
    if (action == "read" || action == "get") {
      return handleRead(repo, payload);
    }
    if (action == "read_by_email" || action == "get_by_email") {
      return handleReadByEmail(repo, payload);
    }
    if (action == "update") {
      return handleUpdate(repo, payload);
    }
    if (action == "delete" || action == "remove") {
      return handleDelete(repo, payload);
    }

    // Custom AA::Data::student methods
    if (action == "load_schedule" || action == "get_schedule" ||
        action == "schedule") {
      return handleLoadSchedule(repo, payload);
    }
    if (action == "load_open_tests" || action == "get_open_tests" ||
        action == "open_tests") {
      return handleLoadOpenTests(repo, payload);
    }
    if (action == "get_assignments" || action == "assignments") {
      return handleGetAssignments(repo, payload);
    }
    if (action == "return_test" || action == "submit_test") {
      return handleReturnTest(repo, payload);
    }

    return error(action, "Unsupported student action: " + action);
  }

 private:
  static AA::Student extractStudent(const nlohmann::json& payload) {
    if (payload.contains("student") && payload["student"].is_object()) {
      return payload.at("student").get<AA::Student>();
    }
    return payload.get<AA::Student>();
  }

  static std::string resolveIdColumn(const nlohmann::json& payload) {
    if (payload.contains("id_column") && !payload["id_column"].is_null()) {
      return requireField<std::string>(payload, "id_column");
    }
    return "student_id";
  }

  static std::string resolveIdValue(const nlohmann::json& payload,
                                    const std::string& idColumn) {
    if (payload.contains(idColumn) && !payload[idColumn].is_null()) {
      return requireField<std::string>(payload, idColumn);
    }
    if (payload.contains("student") && payload["student"].is_object()) {
      const auto& nested = payload.at("student");
      if (nested.contains(idColumn) && !nested[idColumn].is_null()) {
        return nested.at(idColumn).get<std::string>();
      }
    }
    return requireField<std::string>(payload, "student_id");
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

  // --- Base DataBase CRUD ---

  nlohmann::json handleCreate(AA::Data::student& repo,
                              const nlohmann::json& payload) const {
    const AA::Student student = extractStudent(payload);
    if (!repo.save(student)) {
      return error("create", "Failed to save student record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student"] = student;
    return ok("create", std::move(result));
  }

  nlohmann::json handleRead(AA::Data::student& repo,
                            const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Student student{};
    if (!repo.read(idColumn, idValue, student)) {
      return error("read", "Student not found");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student"] = student;
    return ok("read", std::move(result));
  }

  nlohmann::json handleReadByEmail(AA::Data::student& repo,
                                   const nlohmann::json& payload) const {
    const std::string email = requireField<std::string>(payload, "email_addr");

    AA::Student student{};
    if (!repo.loadStudentByEmail(email, student)) {
      return error("read_by_email", "Student not found for email");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student"] = student;
    return ok("read_by_email", std::move(result));
  }

  nlohmann::json handleUpdate(AA::Data::student& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Student existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("update", "Student not found");
    }

    const AA::Student updated = extractStudent(payload);
    if (!repo.update(updated, idColumn, idValue)) {
      return error("update", "Failed to update student record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student"] = updated;
    return ok("update", std::move(result));
  }

  nlohmann::json handleDelete(AA::Data::student& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    if (!repo.remove(idColumn, idValue)) {
      return error("delete", "Student not found or could not be deleted");
    }
    return ok("delete", {{"deleted", true}, {idColumn, idValue}});
  }

  // --- Custom AA::Data::student Methods ---

  nlohmann::json handleLoadSchedule(AA::Data::student& repo,
                                    const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");

    std::vector<std::tuple<std::string, unsigned int>> schedule;
    if (!repo.loadStudentSchedule(studentId, schedule)) {
      return error("load_schedule", "Failed to load student schedule");
    }

    nlohmann::json entries = nlohmann::json::array();
    for (const auto& [teacherId, classPeriod] : schedule) {
      entries.push_back(
          {{"teacher_id", teacherId}, {"class_period", classPeriod}});
    }

    nlohmann::json result = nlohmann::json::object();
    result["student_id"] = studentId;
    result["schedule"] = std::move(entries);
    return ok("load_schedule", std::move(result));
  }

  nlohmann::json handleLoadOpenTests(AA::Data::student& repo,
                                     const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");

    std::vector<AA::OpenTest> openTests;
    if (!repo.loadOpenTestsForStudent(studentId, openTests)) {
      return error("load_open_tests", "Failed to load open tests for student");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student_id"] = studentId;
    result["open_tests"] = openTests;
    return ok("load_open_tests", std::move(result));
  }

  nlohmann::json handleGetAssignments(AA::Data::student& repo,
                                      const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");

    std::vector<AA::GradeRecord> assignments;
    if (!repo.getStudentAssignments(studentId, assignments)) {
      return error("get_assignments", "Failed to load student assignments");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student_id"] = studentId;
    result["assignments"] = assignments;
    return ok("get_assignments", std::move(result));
  }

  nlohmann::json handleReturnTest(AA::Data::student& repo,
                                  const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const int openTestId = requireField<int>(payload, "open_test_id");
    const DT::Date submissionDate = parseDateField(payload, "submission_date");
    const double score = requireField<double>(payload, "score");
    const std::string status = requireField<std::string>(payload, "status");

    const std::string returnTest =
        payload.contains("return_test") && !payload["return_test"].is_null()
            ? payload["return_test"].get<std::string>()
            : std::string{};
    const std::string wrongAnswers =
        payload.contains("wrong_answers") && !payload["wrong_answers"].is_null()
            ? payload["wrong_answers"].get<std::string>()
            : std::string{};
    const float grade = payload.contains("grade") && !payload["grade"].is_null()
                            ? payload["grade"].get<float>()
                            : 0.0f;

    if (!repo.returnStudentTest(studentId, openTestId, submissionDate, score,
                                status, returnTest, wrongAnswers, grade)) {
      return error("return_test", "Failed to record returned student test");
    }

    nlohmann::json result = nlohmann::json::object();
    result["student_id"] = studentId;
    result["open_test_id"] = openTestId;
    result["status"] = status;
    result["score"] = score;
    result["grade"] = grade;
    return ok("return_test", std::move(result));
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_STUDENT_HANDLER_H
