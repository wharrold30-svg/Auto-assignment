#ifndef SQL_STUDENT_H
#define SQL_STUDENT_H

#include <mysql++/mysql++.h>

#include <string>
#include <tuple>
#include <vector>

#include "GradeRecord.h"
#include "OpenTest.h"
#include "Student.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class student : public DataBase<student, AA::Student> {
 public:
  student(mysqlpp::Connection& conn) : DataBase<student, AA::Student>{conn} {}
  student(const student&) = delete;
  student& operator=(const student&) = delete;
  student(const student&&) = delete;
  student& operator=(const student&&) = delete;
  ~student() = default;

  virtual std::string getTableName() const override { return "students"; }

  bool loadStudentSchedule(
      const std::string& studentId,
      std::vector<std::tuple<std::string, unsigned int>>& out_schedule);

  bool loadOpenTestsForStudent(const std::string& studentId,
                               std::vector<AA::OpenTest>& out_tests);

  bool getStudentAssignments(const std::string& studentId,
                             std::vector<AA::GradeRecord>& out_assignments);

  bool returnStudentTest(const std::string& studentId, int openTestId,
                         const DT::Date& submissionDate, double score,
                         const std::string& status,
                         const std::string& returnTest = "",
                         const std::string& wrongAnswers = "",
                         float grade = 0.0f);

  bool loadStudentByEmail(const std::string& email,
                          AA::Student& out_student) const;

};  // student
}  // namespace Data
}  // namespace AA
#endif
