#ifndef STUDENT_H
#define STUDENT_H

#include <algorithm>
#include <compare>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "Date.h"
#include "nlohmann/json.hpp"

namespace AA {
class Student {
 private:
  class Test {
   public:
    Test(const std::string& tName, const float g, DT::Date d)
        : testName{tName}, grade{g}, date{d} {}
    std::string testName;
    float grade;
    DT::Date date;

    std::partial_ordering operator<=>(const Test& rhs) const {
      // First compare by test name
      if (auto cmp = testName <=> rhs.testName; cmp != 0) {
        return cmp;
      }
      // If names are equal, compare by date
      return date <=> rhs.date;
    }

    // 2. Equality operator == (Required explicitly when customizing <=>)
    bool operator==(const Test& rhs) const {
      return (testName == rhs.testName) && (date == rhs.date);
    }
  };
  std::string m_studentID;
  std::string m_emailAddr;
  std::string m_lName;
  std::string m_fName;
  std::size_t m_classPeriod = 0;
  std::vector<Test> m_tests;
  std::string m_school;

 public:
  Student() = default;
  Student(std::string_view studentID, std::string_view emailAddr,
          std::string_view lName, std::string_view fName,
          const std::size_t classPeriod = 0, std::string_view school = "")
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
  const std::string& getSchool() const { return m_school; }
  const std::vector<Test>& getTests() const { return m_tests; }

  // setters
  void setStudentID(std::string_view ID) { m_studentID = ID; }

  void setEmailAddr(std::string_view Addr) { m_emailAddr = Addr; }
  void setLName(std::string_view lName) { m_lName = lName; }
  void setFName(std::string_view fName) { m_fName = fName; }
  void setClassPeriod(const std::size_t period) { m_classPeriod = period; }
  void setSchool(std::string_view school) { m_school = school; }

  void addTest(const std::string& testname, const float grade,
               const DT::Date& date) {
    m_tests.emplace_back(testname, grade, date);
  }
  // clear the object
  void clear() { *this = Student{}; }

  // rmove a test
  void removeTest(const std::string& tName, const DT::Date& d) {
    m_tests.erase(std::remove_if(m_tests.begin(), m_tests.end(),
                                 [&tName, &d](const Test& t) {
                                   return (t.testName == tName) &&
                                          (t.date == d);
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

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Student, m_studentID, m_emailAddr, m_lName,
                                 m_fName, m_classPeriod, m_school)
};  // Student
}  // namespace AA

inline std::ostream& operator<<(std::ostream& os, const AA::Student& student) {
  os << std::format("{}\n{}\n{}\n{}\n{}\n", student.getStudentID(),
                    student.getEmailAddr(), student.getName(),
                    student.getClassPeriod(), student.getSchool());
  return os;
}
#endif
