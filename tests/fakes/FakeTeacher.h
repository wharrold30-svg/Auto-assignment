#ifndef TESTS_FAKES_FAKE_TEACHER_H
#define TESTS_FAKES_FAKE_TEACHER_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "FakeDataBase.h"
#include "OpenTest.h"
#include "Teacher.h"
#include "Test.h"

namespace AA {
namespace Data {
namespace Fakes {

class teacher : public FakeDataBase<AA::Teacher, std::string> {
 public:
  using RosterEntry = std::pair<std::string, unsigned int>;

 private:
  struct Credentials {
    std::string salt;
    std::string password;
  };

  std::unordered_map<std::string, Credentials> m_creds;
  std::unordered_map<std::string, std::vector<AA::Test>> m_testsByTeacher;
  std::unordered_map<std::string, std::vector<AA::OpenTest>>
      m_openTestsByTeacher;
  std::unordered_map<std::string, std::vector<RosterEntry>> m_rostersByTeacher;
  std::unordered_map<int, std::vector<std::string>> m_openTestAssignments;

 public:
  explicit teacher(KeyExtractor extractKey =
                       [](const AA::Teacher& t) { return t.getTeacherID(); })
      : FakeDataBase<AA::Teacher, std::string>{"teachers",
                                               std::move(extractKey)} {}

  teacher(const teacher&) = delete;
  teacher& operator=(const teacher&) = delete;
  teacher(teacher&&) = delete;
  teacher& operator=(teacher&&) = delete;
  ~teacher() override = default;

  std::string getTableName() const override { return "teachers"; }

  // Seed helpers for setting up test fixtures without a DB
  void seedTests(const std::string& teacherId, std::vector<AA::Test> tests) {
    m_testsByTeacher[teacherId] = std::move(tests);
  }

  void seedOpenTests(const std::string& teacherId,
                     std::vector<AA::OpenTest> openTests) {
    m_openTestsByTeacher[teacherId] = std::move(openTests);
  }

  const std::vector<std::string>& assignedStudentsForOpenTest(
      int openTestId) const {
    static const std::vector<std::string> kEmpty;
    auto it = m_openTestAssignments.find(openTestId);
    return (it == m_openTestAssignments.end()) ? kEmpty : it->second;
  }

  // Matches AA::Data::teacher methods
  bool loadCredentials(const std::string& teacherId, std::string& out_salt,
                       std::string& out_password) const {
    auto it = m_creds.find(teacherId);
    if (it == m_creds.end()) return false;
    out_salt = it->second.salt;
    out_password = it->second.password;
    return true;
  }

  bool saveCredentials(const std::string& teacherId, const std::string& salt,
                       const std::string& password) {
    if (teacherId.empty()) return false;
    m_creds[teacherId] = Credentials{salt, password};
    return true;
  }

  bool loadTestsForTeacher(const std::string& teacherId,
                           std::vector<AA::Test>& out_tests) const {
    auto it = m_testsByTeacher.find(teacherId);
    if (it == m_testsByTeacher.end()) return false;
    out_tests = it->second;
    return true;
  }

  bool assignOpenTestToMultipleStudents(
      const std::vector<std::string>& studentIds, int openTestId) {
    if (studentIds.empty() || openTestId <= 0) return false;
    auto& assigned = m_openTestAssignments[openTestId];
    assigned.insert(assigned.end(), studentIds.begin(), studentIds.end());
    return true;
  }

  bool loadOpenTestsForTeacher(const std::string& teacherId,
                               std::vector<AA::OpenTest>& out_tests) const {
    auto it = m_openTestsByTeacher.find(teacherId);
    if (it == m_openTestsByTeacher.end()) return false;
    out_tests = it->second;
    return true;
  }

  bool loadTeacherRoster(const std::string& teacherId,
                         std::vector<RosterEntry>& out_roster) const {
    auto it = m_rostersByTeacher.find(teacherId);
    if (it == m_rostersByTeacher.end()) return false;
    out_roster = it->second;
    return true;
  }

  bool saveTeacherRoster(const std::string& teacherId,
                         const std::vector<RosterEntry>& studentPeriodPairs) {
    if (teacherId.empty()) return false;
    m_rostersByTeacher[teacherId] = studentPeriodPairs;
    return true;
  }
};

}  // namespace Fakes
}  // namespace Data
}  // namespace AA

#endif
