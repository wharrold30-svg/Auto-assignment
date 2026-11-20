#ifndef SQL_SCHOOL_H
#define SQL_SCHOOL_H

#include <mysql++/mysql++.h>

#include <string>
#include <vector>

#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class school : public DataBase<school, AA::School> {
 public:
  school(mysqlpp::Connection& conn) : DataBase<school, AA::School>{conn} {}
  school(const school&) = delete;
  school& operator=(const school&) = delete;
  school(const school&&) = delete;
  school& operator=(const school&&) = delete;
  ~school() = default;

  virtual std::string getTableName() const override { return "schools"; }

  bool transferStudentSchool(const std::string& studentId,
                             const std::string& newSchoolName);

  bool removeStudentFromSchoolCompletely(const std::string& studentId,
                                         const std::string& schoolName);

  bool transferTeacherSchool(const std::string& teacherId,
                             const std::string& newSchoolName);

  bool removeTeacherFromSchool(const std::string& teacherId);

  bool loadTeachersForSchool(const std::string& schoolName,
                             std::vector<AA::Teacher>& out_teachers) const;

  bool loadStudentsForSchool(const std::string& schoolName,
                             std::vector<AA::Student>& out_students) const;

};  // school
}  // namespace Data
}  // namespace AA
#endif
