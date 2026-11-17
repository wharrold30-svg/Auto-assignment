#ifndef STUDENT_H
#define STUDENT_H

#include <algorithm>
#include <compare>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "Date.h"
#include "School.h"
#include "nlohmann/json.hpp"
#include "sql/SSQLS.h"

namespace AA {
class Student {
 private:
  class test {
   private:
    std::string m_testName;
    float m_grade;
    DT::Date m_date;

   public:
    test() = default;
    test(const std::string& tName, const float g, DT::Date d)
        : m_testName{tName}, m_grade{g}, m_date{d} {}

    // getters
    const std::string& getTestName() const { return m_testName; }
    float getGrade() const { return m_grade; }
    const DT::Date& getDate() const { return m_date; }

    std::partial_ordering operator<=>(const test& rhs) const {
      // First compare by test name
      if (auto cmp = m_testName <=> rhs.m_testName; cmp != 0) {
        return cmp;
      }
      // If names are equal, compare by date
      return m_date <=> rhs.m_date;
    }

    // 2. Equality operator == (Required explicitly when customizing <=>)
    bool operator==(const test& rhs) const {
      return (m_testName == rhs.m_testName) && (m_date == rhs.m_date);
    }

    // Enable JSON serialization for test
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(test, m_testName, m_grade, m_date)
  };  // test
  std::string m_studentID;
  std::string m_emailAddr;
  std::string m_lName;
  std::string m_fName;
  std::size_t m_classPeriod = 0;
  std::vector<test> m_tests;
  School m_school;

 public:
  Student() = default;
  Student(std::string_view studentID, std::string_view emailAddr,
          std::string_view lName, std::string_view fName,
          const std::size_t classPeriod, const School& school)
      : m_studentID{studentID},
        m_emailAddr{emailAddr},
        m_lName{lName},
        m_fName{fName},
        m_classPeriod{classPeriod},
        m_school{school} {}

  // getters
  const std::string& getStudentID() const { return m_studentID; }

  const std::string& getEmailAddr() const { return m_emailAddr; }
  const std::string& getLName() const { return m_lName; }
  const std::string& getFName() const { return m_fName; }
  const std::string getName() const {
    return std::format("{} {}", getLName(), getFName());
  }
  std::size_t getClassPeriod() const { return m_classPeriod; }
  const School& getSchool() const { return m_school; }
  const std::vector<test>& getTests() const { return m_tests; }

  // setters
  void setStudentID(std::string_view ID) { m_studentID = ID; }

  void setEmailAddr(std::string_view Addr) { m_emailAddr = Addr; }
  void setLName(std::string_view lName) { m_lName = lName; }
  void setFName(std::string_view fName) { m_fName = fName; }
  void setClassPeriod(const std::size_t period) { m_classPeriod = period; }
  void setSchool(const School& school) { m_school = school; }

  void addTest(const std::string& testname, const float grade,
               const DT::Date& date) {
    m_tests.emplace_back(testname, grade, date);
  }
  // clear the object
  void clear() { *this = Student{}; }

  // rmove a test
  void removeTest(const std::string& tName, const DT::Date& d) {
    m_tests.erase(std::remove_if(m_tests.begin(), m_tests.end(),
                                 [&tName, &d](const test& t) {
                                   return (t.getTestName() == tName) &&
                                          (t.getDate() == d);
                                 }),
                  m_tests.end());
  }
  // spaceshipand equal-to  operstors overload so thatstudents
  // are sorted based on their class period and name
  std::strong_ordering operator<=>(const Student& rhs) const {
    if (const auto order = (m_classPeriod <=> rhs.m_classPeriod); order != 0) {
      return order;
    }
    return getName() <=> rhs.getName();
  }
  bool operator==(const Student& rhs) const {
    return m_classPeriod == rhs.m_classPeriod && m_studentID == rhs.m_studentID;
  }

  // When writing back to SSQLS, we extract the school's name for the foreign
  // key column
  SSQLS::students toSSQLS() const {
    SSQLS::students row;
    row.student_id = m_studentID;
    row.email_addr = m_emailAddr;
    row.l_name = m_lName;
    row.f_name = m_fName;
    row.class_period = static_cast<unsigned int>(m_classPeriod);
    row.school_name = m_school.getName();

    // Serialize std::vector<test> into a JSON string using nlohmann::json
    nlohmann::json j_tests = m_tests;
    // Note: Ensure your SSQLS::students definition includes a text field
    // matching this, or handle it accordingly in your Data repository layer.
    row.tests_json = j_tests.dump();

    return row;
  }

  static Student fromSSQLS(const SSQLS::students& row,
                           const School& school = School{}) {
    // 1. Deserialize tests_json string back into std::vector<Test>
    std::vector<test> tests;
    if (!row.tests_json.empty()) {
      nlohmann::json j = nlohmann::json::parse(row.tests_json, nullptr, false);
      if (!j.is_discarded() && j.is_array()) {
        tests = j.get<std::vector<test>>();
      }
    }

    // 2. Construct base student profile
    Student student(row.student_id, row.email_addr, row.l_name, row.f_name,
                    static_cast<std::size_t>(row.class_period), school);

    // 3. Attach the deserialized test vector
    student.m_tests = std::move(tests);
    return student;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Student, m_studentID, m_emailAddr, m_lName,
                                 m_fName, m_classPeriod, m_school, m_tests)
};  // Student
}  // namespace AA

inline std::ostream& operator<<(std::ostream& os, const AA::Student& student) {
  os << std::format("{}\n{}\n{}\n{}\n{}\n", student.getStudentID(),
                    student.getEmailAddr(), student.getName(),
                    student.getClassPeriod(), student.getSchool().getName());
  return os;
}
#endif
