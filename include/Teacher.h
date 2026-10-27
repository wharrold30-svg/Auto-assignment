#ifndef TEACHER_H
#define TEACHER_H
#include <compare>
#include <stdexcept>
#include <string>
#include <string_view>

#include "School.h"
#include "nlohmann/json.hpp"

namespace AA {
class Teacher {
 private:
  std::string m_lName;
  std::string m_fName;
  std::string m_teacherID;
  std::string m_emailAddr;
  School m_school;

 public:
  Teacher() = default;
  Teacher(std::string_view lName, std::string_view fName,
          std::string_view teacherID, std::string_view emailAddr,
          const School& school)
      : m_lName{lName},
        m_fName{fName},
        m_teacherID{teacherID},
        m_emailAddr{emailAddr},
        m_school{school} {}

  // getters
  inline const std::string& getLName() const { return m_lName; }
  inline const std::string& getFName() const { return m_fName; }
  inline const std::string& getTeacherID() const { return m_teacherID; }
  inline const std::string& getEmailAddr() const { return m_emailAddr; }
  inline const std::string getName() const { return m_lName + ", " + m_fName; }
  inline const School& getSchool() const { return m_school; }

  // setters
  void setLName(std::string_view lName) { m_lName = lName; }
  void setFName(std::string_view fName) { m_fName = fName; }
  void setEmailAddr(std::string_view emailAddr) { m_emailAddr = emailAddr; }
  void setTeacherID(std::string_view teacherID) { m_teacherID = teacherID; }
  void setSchool(const School& school) { m_school = school; }

  // clear
  inline void clear() { *this = Teacher{}; }

  // spaceship operator overload sothat teacher objects are ordered according to
  // their school first then their email address
  std::strong_ordering operator<=>(const Teacher& rhs) {
    if (const auto ret{m_school <=> rhs.m_school}; ret != 0) {
      return ret;
    }
    return m_emailAddr <=> rhs.m_emailAddr;
  }

  // equal-to operator overloads so that two teacher objects are equal if
  // theybhabe the same school, email address, and teacher ID
  bool operator==(const Teacher& rhs) {
    return m_teacherID == rhs.m_teacherID && m_emailAddr == rhs.m_emailAddr &&
           m_school == rhs.m_school;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Teacher, m_lName, m_fName, m_teacherID,
                                 m_emailAddr, m_school)
};  // Teacher
}  // namespace AA

#endif
