#ifndef DATA_H
#define DATA_H
#include <string>
#include <string_view>

#include "GradeRecord.h"
#include "OpenTest.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "Test.h"
#include "mysql++/mysql++.h"
#include "nlohmann/json.hpp"
#include "sql/IDataRepository.h"

namespace AA {
namespace Data {
class Data : public IDataRepository {
 protected:
  mysqlpp::Connection m_conn;
  nlohmann::json m_results;

 private:
  void getDBCreds(std::string& server, std::string& user,
                  std::string& password);
  void Init();
  bool initializeDatabase(const std::string& server, const std::string& user,
                          const std::string& password,
                          const std::string& dbName = "auto_assignment");

 public:
  Data();
  ~Data();

  bool saveStudent(const Student& student) override;
  bool loadStudent(const std::string& student_id,
                   Student& out_student) override;
  bool saveTest(const Test& test) override;
  bool loadTest(const std::string& test_id, Test& out_test) override;

  bool saveTeacher(const AA::Teacher& teacher) override;

  bool loadTeacher(const std::string& teacher_id,
                   AA::Teacher& out_teacher) override;

  bool saveSchool(const AA::School& school) override;
  bool loadSchool(const std::string& school_name,
                  AA::School& out_school) override;

  bool enrollStudentInClass(const std::string& teacherId,
                            const std::string& studentId,
                            unsigned int classPeriod) override;

  bool saveTeacherRoster(
      const std::string& teacherId,
      const std::vector<std::pair<std::string, unsigned int>>&
          studentPeriodPairs) override;

  bool loadTeacherRoster(
      const std::string& teacherId,
      std::vector<std::pair<std::string, unsigned int>>& out_roster);

  bool loadStudentSchedule(
      const std::string& studentId,
      std::vector<std::tuple<std::string, unsigned int>>& out_schedule);
  bool saveOpenTest(const AA::OpenTest& openTest);

  bool loadOpenTestsByDateRange(const DT::Date& startDate,
                                const DT::Date& endDate,
                                std::vector<OpenTest>& out_tests);

  bool loadOpenTestsForTeacher(const std::string& teacherId,
                               std::vector<OpenTest>& out_tests);
  bool loadOpenTestsForStudent(const std::string& studentId,
                               std::vector<OpenTest>& out_tests);

  bool assignOpenTestToStudent(const std::string& studentId, int openTestId);
  bool assignOpenTestToMultipleStudents(
      const std::vector<std::string>& studentIds, int openTestId);
  bool returnStudentTest(const std::string& studentId, int openTestId,
                         const DT::Date& submissionDate, double score,
                         const std::string& status);

  bool loadTestsForTeacher(const std::string& teacherId,
                           std::vector<AA::Test>& out_tests);

  bool loadTestsPaginated(int limit, int offset,
                          std::vector<AA::Test>& out_tests);

  bool getStudentAssignments(const std::string& studentId,
                             std::vector<GradeRecord>& out_assignments);

  bool removeStudentFromClass(const std::string& studentId,
                              const std::string& teacherId, int classPeriod);

  bool removeStudentFromSchoolCompletely(const std::string& studentId,
                                         const std::string& schoolName);

  bool transferStudentSchool(const std::string& studentId,
                             const std::string& newSchoolName);
};  // Data
}  // namespace Data
}  // namespace AA
#endif  // DATA_H
