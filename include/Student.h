#ifndef STUDENT_H
#define STUDENT_H

#include <compare>
#include <iostream>
#include <string>
#include <string_view>

#include "nlohmann/json.h"

namespace AA {
class Student {
 private:
  std::string m_studentID;
  std::string m_emailAddr;
  std::string m_lName;
  std::string m_fName;
  std::size_t m_classPeriod;
  std::string m_school;

 public:
  Student() = default;
  Student(std::string_view studentID, std::string_view emailAddr,
          std::string_view lName, std::string_view fName,
          const std::size_t classPeriod = 0, std::string_view school = "")
      : m_studentID{studentID},
        m_emailAddr{emailAddr},
        m_lName{lname},
        m_fName{fName},
        m_classPeriod{classPeriod},
        m_school{school} {}

  // getters
  const std::string& getStudentID() const { return m_studentID; }

  const std::string& getEmailAddr() const { return m_emailAddr; }
  const std::string& getLName() const { return m_lName; }
  const std::string& getFName() const { return m_fName; }
  const std::string& getName() const {
    return std::format("{} {}", getLName(), getFName());
  }
  std::size_t getClassPeriod() const { return m_classPeriod; }
  const std::string& getSchool() const { return m_school; }

  // setters
  void setStudentID(std::string_view ID) { m_studentID = ID; }

  void setEmailAddr(std::string_view Addr) { m_emailAddr = Addr; }
  void setLName(std::string_view lName) { m_lName = lName; }
  void setFName(std::string_view fName) { m_fName = fName; }
  void setClassPeriod(const std::size_t period) { m_classPeriod = period; }
  void setSchool(std::string_view school) { m_school = school; }

  // clear the object
  void clear() { *this = Student{}; }

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

std::ostream& operator<<(std::ostream& os, const AA::Student& student) {
  os << std::format("{}\n{}\n{}\n{}\n{}\n", student;
                    getStudentID(), student.getEmailAddr(), student.getName(),
                    student.getClassPeriod(), getSchool());
  return os;
}
#endif
