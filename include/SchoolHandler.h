#ifndef AA_SCHOOL_HANDLER_H
#define AA_SCHOOL_HANDLER_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "RequestHandler.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "sql/school.h"

namespace AA::Server::Request {

class SchoolHandler final : public Handler {
 public:
  using Handler::Handler;

 protected:
  std::string_view entityName() const override { return "school"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& conn) override {
    AA::Data::school repo{conn};

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

    // School member queries
    if (action == "load_teachers" || action == "get_teachers" ||
        action == "teachers") {
      return handleLoadTeachers(repo, payload);
    }
    if (action == "load_students" || action == "get_students" ||
        action == "students") {
      return handleLoadStudents(repo, payload);
    }

    // Student & teacher school transfers and removals
    if (action == "transfer_student") {
      return handleTransferStudent(repo, payload);
    }
    if (action == "remove_student") {
      return handleRemoveStudent(repo, payload);
    }
    if (action == "transfer_teacher") {
      return handleTransferTeacher(repo, payload);
    }
    if (action == "remove_teacher") {
      return handleRemoveTeacher(repo, payload);
    }

    return error(action, "Unsupported school action: " + action);
  }

 private:
  static AA::School extractSchool(const nlohmann::json& payload) {
    if (payload.contains("school") && payload["school"].is_object()) {
      return payload.at("school").get<AA::School>();
    }
    return payload.get<AA::School>();
  }

  static std::string resolveIdColumn(const nlohmann::json& payload) {
    if (payload.contains("id_column") && !payload["id_column"].is_null()) {
      const std::string col = requireField<std::string>(payload, "id_column");
      if (col != "name") {
        throw std::invalid_argument("Unsupported id_column: " + col);
      }
      return col;
    }
    return "name";
  }

  static std::string resolveIdValue(const nlohmann::json& payload,
                                    const std::string& idColumn,
                                    bool allowRenameKeys = false) {
    if (allowRenameKeys) {
      for (const char* key : {"old_name", "target_name"}) {
        if (payload.contains(key) && !payload[key].is_null()) {
          return requireField<std::string>(payload, key);
        }
      }
    }

    if (payload.contains(idColumn) && !payload[idColumn].is_null()) {
      return requireField<std::string>(payload, idColumn);
    }
    if (payload.contains("school") && payload["school"].is_object()) {
      const auto& nested = payload.at("school");
      if (nested.contains(idColumn) && !nested[idColumn].is_null()) {
        return requireField<std::string>(nested, idColumn);
      }
    }
    return requireField<std::string>(payload, "name");
  }

  static std::string resolveSchoolName(const nlohmann::json& payload) {
    if (payload.contains("school_name") && !payload["school_name"].is_null()) {
      return requireField<std::string>(payload, "school_name");
    }
    if (payload.contains("name") && !payload["name"].is_null()) {
      return requireField<std::string>(payload, "name");
    }
    if (payload.contains("school") && payload["school"].is_object()) {
      const auto& nested = payload.at("school");
      if (nested.contains("school_name") && !nested["school_name"].is_null()) {
        return requireField<std::string>(nested, "school_name");
      }
      if (nested.contains("name") && !nested["name"].is_null()) {
        return requireField<std::string>(nested, "name");
      }
    }
    throw std::invalid_argument("Missing required field: school_name or name");
  }

  // --- Base DataBase CRUD ---

  nlohmann::json handleCreate(AA::Data::school& repo,
                              const nlohmann::json& payload) const {
    const AA::School schoolObj = extractSchool(payload);
    if (!repo.save(schoolObj)) {
      return error("create", "Failed to save school record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school"] = schoolObj;
    return ok("create", std::move(result));
  }

  nlohmann::json handleRead(AA::Data::school& repo,
                            const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::School schoolObj{};
    if (!repo.read(idColumn, idValue, schoolObj)) {
      return error("read", "School not found");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school"] = schoolObj;
    return ok("read", std::move(result));
  }

  nlohmann::json handleUpdate(AA::Data::school& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue =
        resolveIdValue(payload, idColumn, /*allowRenameKeys=*/true);

    AA::School existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("update", "School not found");
    }

    const AA::School updated = extractSchool(payload);
    if (!repo.update(updated, idColumn, idValue)) {
      return error("update", "Failed to update school record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school"] = updated;
    return ok("update", std::move(result));
  }

  nlohmann::json handleDelete(AA::Data::school& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::School existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("delete", "School not found");
    }

    if (!repo.remove(idColumn, idValue)) {
      return error("delete", "Failed to delete school record");
    }
    return ok("delete", {{"deleted", true}, {idColumn, idValue}});
  }

  // --- School Member Queries ---

  nlohmann::json handleLoadTeachers(AA::Data::school& repo,
                                    const nlohmann::json& payload) const {
    const std::string schoolName = resolveSchoolName(payload);

    std::vector<AA::Teacher> teachers;
    if (!repo.loadTeachersForSchool(schoolName, teachers)) {
      return error("load_teachers", "Failed to load teachers for school");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school_name"] = schoolName;
    result["teachers"] = teachers;
    return ok("load_teachers", std::move(result));
  }

  nlohmann::json handleLoadStudents(AA::Data::school& repo,
                                    const nlohmann::json& payload) const {
    const std::string schoolName = resolveSchoolName(payload);

    std::vector<AA::Student> students;
    if (!repo.loadStudentsForSchool(schoolName, students)) {
      return error("load_students", "Failed to load students for school");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school_name"] = schoolName;
    result["students"] = students;
    return ok("load_students", std::move(result));
  }

  // --- Student & Teacher Transfers / Removals ---

  nlohmann::json handleTransferStudent(AA::Data::school& repo,
                                       const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const std::string newSchoolName =
        requireField<std::string>(payload, "new_school_name");

    if (!repo.transferStudentSchool(studentId, newSchoolName)) {
      return error("transfer_student",
                   "Failed to transfer student to new school");
    }

    return ok("transfer_student",
              {{"student_id", studentId}, {"new_school_name", newSchoolName}});
  }

  nlohmann::json handleRemoveStudent(AA::Data::school& repo,
                                     const nlohmann::json& payload) const {
    const std::string studentId =
        requireField<std::string>(payload, "student_id");
    const std::string schoolName = resolveSchoolName(payload);

    if (!repo.removeStudentFromSchoolCompletely(studentId, schoolName)) {
      return error("remove_student",
                   "Failed to remove student from school completely");
    }

    return ok("remove_student",
              {{"student_id", studentId}, {"school_name", schoolName}});
  }

  nlohmann::json handleTransferTeacher(AA::Data::school& repo,
                                       const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");
    const std::string newSchoolName =
        requireField<std::string>(payload, "new_school_name");

    if (!repo.transferTeacherSchool(teacherId, newSchoolName)) {
      return error("transfer_teacher",
                   "Failed to transfer teacher to new school");
    }

    return ok("transfer_teacher",
              {{"teacher_id", teacherId}, {"new_school_name", newSchoolName}});
  }

  nlohmann::json handleRemoveTeacher(AA::Data::school& repo,
                                     const nlohmann::json& payload) const {
    const std::string teacherId =
        requireField<std::string>(payload, "teacher_id");

    if (!repo.removeTeacherFromSchool(teacherId)) {
      return error("remove_teacher", "Failed to remove teacher from school");
    }

    return ok("remove_teacher", {{"teacher_id", teacherId}, {"removed", true}});
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_SCHOOL_HANDLER_H
