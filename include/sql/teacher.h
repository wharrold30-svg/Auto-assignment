#ifndef SQL_TEACHER_H
#define SQL_TEACHER_H

#include <mysql++/mysql++.h>

#include <string>
#include <vector>

#include "OpenTest.h"
#include "Teacher.h"
#include "Test.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class teacher : public DataBase<teacher, AA::Teacher> {
 public:
  teacher(mysqlpp::Connection& conn) : DataBase<teacher, AA::Teacher>{conn} {}
  teacher(const teacher&) = delete;
  teacher& operator=(const teacher&) = delete;
  teacher(const teacher&&) = delete;
  teacher& operator=(const teacher&&) = delete;
  ~teacher() = default;

  virtual std::string getTableName() const override { return "teachers"; }

  bool loadCredentials(const std::string& teacherId, std::string& out_salt,
                       std::string& out_password);

  bool saveCredentials(const std::string& teacherId, const std::string& salt,
                       const std::string& password);

  bool loadTestsForTeacher(const std::string& teacherId,
                           std::vector<AA::Test>& out_tests);

  bool assignOpenTestToMultipleStudents(
      const std::vector<std::string>& studentIds, int openTestId);

  bool loadOpenTestsForTeacher(const std::string& teacherId,
                               std::vector<AA::OpenTest>& out_tests);

  bool loadTeacherRoster(
      const std::string& teacherId,
      std::vector<std::pair<std::string, unsigned int>>& out_roster);

  bool saveTeacherRoster(
      const std::string& teacherId,
      const std::vector<std::pair<std::string, unsigned int>>&
          studentPeriodPairs);
};  // teacher
}  // namespace Data
}  // namespace AA
#endif
