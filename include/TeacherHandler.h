#ifndef AA_TEACHER_HANDLER_H
#define AA_TEACHER_HANDLER_H

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "OpenTest.h"
#include "RequestHandler.h"
#include "Teacher.h"
#include "Test.h"
#include "sql/teacher.h"

namespace AA::Server::Request {

class TeacherHandler final : public Handler {
 public:
  using Handler::Handler;

 protected:
  std::string_view entityName() const override { return "teacher"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& conn) override {
    AA::Data::teacher repo{conn};

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

    // Credentials
    if (action == "load_credentials" || action == "get_credentials") {
      return handleLoadCredentials(repo, payload);
    }
    if (action == "save_credentials") {
      return handleSaveCredentials(repo, payload);
    }

    // Tests & open tests
    if (action == "load_tests" || action == "get_tests" || action == "tests") {
      return handleLoadTests(repo, payload);
    }
    if (action == "load_open_tests" || action == "get_open_tests" ||
        action == "open_tests") {
      return handleLoadOpenTests(repo, payload);
    }
    if (action == "assign_open_test") {
      return handleAssignOpenTest(repo, payload);
    }
    if (action == "assign_open_test_bulk" || action == "assign_open_tests") {
      return handleAssignOpenTestBulk(repo, payload);
    }

    // Roster & class periods
    if (action == "load_roster" || action == "get_roster" ||
        action == "roster") {
      return handleLoadRoster(repo, payload);
    }
    if (action == "save_roster") {
      return handleSaveRoster(repo, payload);
    }
    if (action == "load_roster_for_period" ||
        action == "get_roster_for_period") {
      return handleLoadRosterForPeriod(repo, payload);
    }
    if (action == "enroll_student" || action == "add_student") {
      return handleEnrollStudent(repo, payload);
    }
    if (action == "remove_student" || action == "unenroll_student") {
      return handleRemoveStudent(repo, payload);
    }

    return error(action, "Unsupported teacher action: " + action);
  }

 private:
  static AA::Teacher extractTeacher(const nlohmann::json& payload) {
    if (payload.contains("teacher") && payload["teacher"].is_object()) {
      return payload.at("teacher").get<AA::Teacher>();
    }
    return payload.get<AA::Teacher>();
  }

  static std::string resolveIdColumn(const nlohmann::json& payload) {
    if (payload.contains("id_column") && !payload["id_column"].is_null()) {
      return requireField<std::string>(payload, "id_column");
    }
    return "teacher_id";
  }

  static std::string resolveIdValue(const nlohmann::json& payload,
                                    const std::string& idColumn) {
    if (payload.contains(idColumn) && !payload[idColumn].is_null()) {
      return requireField<std::string>(payload, idColumn);
    }
    if (payload.contains("teacher") && payload["teacher"].is_object()) {
      const auto& nested = payload.at("teacher");
      if (nested.contains(idColumn) && !nested[idColumn].is_null()) {
        return nested.at(idColumn).get<std::string>();
      }
    }
    return requireField<std::string>(payload, "teacher_id");
  }

  static std::vector<std::pair<std::string, unsigned int>> parseRosterPairs(
      const nlohmann::json& payload) {
    if (!payload.contains("roster") || !payload["roster"].is_array()) {
      throw std::invalid_argument("Missing or non-array 'roster' field");
    }

    std::vector<std::pair<std::string, unsigned int>> pairs;
    pairs.reserve(payload["roster"].size());
    for (const auto& entry : payload["roster"]) {
      if (!entry.is_object()) {
        throw std::invalid_argument("Each roster entry must be a JSON object");
      }
      const auto studentId = requireField<std::string>(entry, "student_id");
      const auto classPeriod =
          requireField<unsigned int>(entry, "class_period");
      pairs.emplace_back(studentId, classPeriod);
    }
    return pairs;
  }

  // --- Base DataBase CRUD ---

  nlohmann::json handleCreate(AA::Data::teacher& repo,
                              const nlohmann::json& payload) const {
    const AA::Teacher teacher = extractTeacher(payload);
    if (!repo.save(teacher)) {
      return error("create", "Failed to save teacher record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher"] = teacher;
    return ok("create", std::move(result));
  }

  nlohmann::json handleRead(AA::Data::teacher& repo,
                            const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Teacher teacher{};
    if (!repo.read(idColumn, idValue, teacher)) {
      return error("read", "Teacher not found");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher"] = teacher;
    return ok("read", std::move(result));
  }

  nlohmann::json handleReadByEmail(AA::Data::teacher& repo,
                                   const nlohmann::json& payload) const {
    const std::string email = requireField<std::string>(payload, "email_addr");

    AA::Teacher teacher{};
    if (!repo.loadTeacherByEmail(email, teacher)) {
      return error("read_by_email", "Teacher not found for email");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher"] = teacher;
    return ok("read_by_email", std::move(result));
  }

  nlohmann::json handleUpdate(AA::Data::teacher& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Teacher existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("update", "Teacher not found");
    }

    const AA::Teacher updated = extractTeacher(payload);
    if (!repo.update(updated, idColumn, idValue)) {
      return error("update", "Failed to update teacher record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher"] = updated;
    return ok("update", std::move(result));
  }

  nlohmann::json handleDelete(AA::Data::teacher& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    if (!repo.remove(idColumn, idValue)) {
      return error("delete", "Teacher not found or could not be deleted");
    }
    return ok("delete", {{"deleted", true}, {idColumn, idValue}});
  }

  // --- Credentials ---

  nlohmann::json handleLoadCredentials(AA::Data::teacher& repo,
                                       const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");

    std::string salt;
    std::string password;
    if (!repo.loadCredentials(teacherId, salt, password)) {
      return error("load_credentials",
                   "Failed to load credentials for teacher");
    }

    return ok("load_credentials", {{"teacher_id", teacherId},
                                   {"salt", std::move(salt)},
                                   {"password", std::move(password)}});
  }

  nlohmann::json handleSaveCredentials(AA::Data::teacher& repo,
                                       const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    const std::string salt = requireField<std::string>(payload, "salt");
    const std::string password = requireField<std::string>(payload, "password");

    if (!repo.saveCredentials(teacherId, salt, password)) {
      return error("save_credentials",
                   "Failed to save credentials for teacher");
    }

    return ok("save_credentials", {{"teacher_id", teacherId}, {"saved", true}});
  }

  // --- Tests & Open Tests ---

  nlohmann::json handleLoadTests(AA::Data::teacher& repo,
                                 const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");

    std::vector<AA::Test> tests;
    if (!repo.loadTestsForTeacher(teacherId, tests)) {
      return error("load_tests", "Failed to load tests for teacher");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher_id"] = teacherId;
    result["tests"] = tests;
    return ok("load_tests", std::move(result));
  }

  nlohmann::json handleLoadOpenTests(AA::Data::teacher& repo,
                                     const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");

    std::vector<AA::OpenTest> openTests;
    if (!repo.loadOpenTestsForTeacher(teacherId, openTests)) {
      return error("load_open_tests", "Failed to load open tests for teacher");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher_id"] = teacherId;
    result["open_tests"] = openTests;
    return ok("load_open_tests", std::move(result));
  }

  nlohmann::json handleAssignOpenTest(AA::Data::teacher& repo,
                                      const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const int openTestId = requireField<int>(payload, "open_test_id");

    if (!repo.assignOpenTestToStudent(studentId, openTestId)) {
      return error("assign_open_test", "Failed to assign open test to student");
    }

    return ok("assign_open_test",
              {{"student_id", studentId}, {"open_test_id", openTestId}});
  }

  nlohmann::json handleAssignOpenTestBulk(AA::Data::teacher& repo,
                                          const nlohmann::json& payload) const {
    const int openTestId = requireField<int>(payload, "open_test_id");
    if (!payload.contains("student_ids") ||
        !payload["student_ids"].is_array()) {
      throw std::invalid_argument("Missing or non-array 'student_ids' field");
    }
    const auto studentIds =
        payload.at("student_ids").get<std::vector<std::string>>();

    if (!repo.assignOpenTestToMultipleStudents(studentIds, openTestId)) {
      return error("assign_open_test_bulk",
                   "Failed to assign open test to students");
    }

    return ok("assign_open_test_bulk",
              {{"open_test_id", openTestId}, {"student_ids", studentIds}});
  }

  // --- Roster & Class Periods ---

  nlohmann::json handleLoadRoster(AA::Data::teacher& repo,
                                  const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");

    std::vector<std::pair<std::string, unsigned int>> roster;
    if (!repo.loadTeacherRoster(teacherId, roster)) {
      return error("load_roster", "Failed to load teacher roster");
    }

    nlohmann::json entries = nlohmann::json::array();
    for (const auto& [studentId, classPeriod] : roster) {
      entries.push_back(
          {{"student_id", studentId}, {"class_period", classPeriod}});
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher_id"] = teacherId;
    result["roster"] = std::move(entries);
    return ok("load_roster", std::move(result));
  }

  nlohmann::json handleSaveRoster(AA::Data::teacher& repo,
                                  const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    auto pairs = parseRosterPairs(payload);

    if (!repo.saveTeacherRoster(teacherId, pairs)) {
      return error("save_roster", "Failed to save teacher roster");
    }

    nlohmann::json entries = nlohmann::json::array();
    for (const auto& [studentId, classPeriod] : pairs) {
      entries.push_back(
          {{"student_id", studentId}, {"class_period", classPeriod}});
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher_id"] = teacherId;
    result["roster"] = std::move(entries);
    return ok("save_roster", std::move(result));
  }

  nlohmann::json handleLoadRosterForPeriod(
      AA::Data::teacher& repo, const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    const unsigned int classPeriod =
        requireField<unsigned int>(payload, "class_period");

    std::vector<std::string> studentIds;
    if (!repo.loadRosterForPeriod(teacherId, classPeriod, studentIds)) {
      return error("load_roster_for_period",
                   "Failed to load roster for class period");
    }

    nlohmann::json result = nlohmann::json::object();
    result["teacher_id"] = teacherId;
    result["class_period"] = classPeriod;
    result["student_ids"] = std::move(studentIds);
    return ok("load_roster_for_period", std::move(result));
  }

  nlohmann::json handleEnrollStudent(AA::Data::teacher& repo,
                                     const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const unsigned int classPeriod =
        requireField<unsigned int>(payload, "class_period");

    if (!repo.enrollStudentInClass(teacherId, studentId, classPeriod)) {
      return error("enroll_student", "Failed to enroll student in class");
    }

    return ok("enroll_student", {{"teacher_id", teacherId},
                                 {"student_id", studentId},
                                 {"class_period", classPeriod}});
  }

  nlohmann::json handleRemoveStudent(AA::Data::teacher& repo,
                                     const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const unsigned int classPeriod =
        requireField<unsigned int>(payload, "class_period");

    if (!repo.removeStudentFromClass(teacherId, studentId, classPeriod)) {
      return error("remove_student", "Failed to remove student from class");
    }

    return ok("remove_student", {{"teacher_id", teacherId},
                                 {"student_id", studentId},
                                 {"class_period", classPeriod}});
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_TEACHER_HANDLER_H
