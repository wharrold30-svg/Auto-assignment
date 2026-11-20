#ifndef FAKE_SCHOOL_H
#define FAKE_SCHOOL_H

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "FakeStore.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"

namespace AA::Fake {

class FakeSchool {
 private:
  Store& m_store;

 public:
  explicit FakeSchool(Store& store) : m_store(store) {}

  bool loadTeachersForSchool(std::string_view schoolName,
                             std::vector<AA::Teacher>& out_teachers) const {
    out_teachers.clear();
    if (schoolName.empty()) {
      return false;
    }
    for (const auto& [teacherId, teacher] : m_store.teachers) {
      if (teacher.getSchool().getName() == schoolName) {
        out_teachers.push_back(teacher);
      }
    }
    return true;
  }

  bool loadStudentsForSchool(std::string_view schoolName,
                             std::vector<AA::Student>& out_students) const {
    out_students.clear();
    if (schoolName.empty()) {
      return false;
    }
    for (const auto& [studentId, student] : m_store.students) {
      if (student.getSchool().getName() == schoolName) {
        out_students.push_back(student);
      }
    }
    return true;
  }

  bool transferTeacherSchool(std::string_view teacherId,
                             std::string_view newSchoolName) {
    const auto tIt = m_store.teachers.find(std::string(teacherId));
    if (tIt == m_store.teachers.end() || newSchoolName.empty()) {
      return false;
    }
    AA::School targetSchool;
    const auto sIt = m_store.schools.find(std::string(newSchoolName));
    if (sIt != m_store.schools.end()) {
      targetSchool = sIt->second;
    } else {
      targetSchool.setName(newSchoolName);
    }
    tIt->second.setSchool(targetSchool);
    return true;
  }

  bool removeTeacherFromSchool(std::string_view teacherId) {
    const std::string tKey(teacherId);
    const auto tIt = m_store.teachers.find(tKey);
    if (tIt == m_store.teachers.end()) {
      return false;
    }
    tIt->second.setSchool(AA::School{});
    m_store.roster.erase(
        std::remove_if(
            m_store.roster.begin(), m_store.roster.end(),
            [&](const RosterEntry& entry) { return entry.teacherId == tKey; }),
        m_store.roster.end());
    return true;
  }

  bool transferStudentSchool(std::string_view studentId,
                             std::string_view newSchoolName) {
    const auto stIt = m_store.students.find(std::string(studentId));
    if (stIt == m_store.students.end() || newSchoolName.empty()) {
      return false;
    }
    AA::School targetSchool;
    const auto sIt = m_store.schools.find(std::string(newSchoolName));
    if (sIt != m_store.schools.end()) {
      targetSchool = sIt->second;
    } else {
      targetSchool.setName(newSchoolName);
    }
    stIt->second.setSchool(targetSchool);
    return true;
  }

  bool removeStudentFromSchoolCompletely(std::string_view studentId,
                                         std::string_view schoolName) {
    const std::string sKey(studentId);
    const auto stIt = m_store.students.find(sKey);
    if (stIt == m_store.students.end()) {
      return false;
    }
    if (stIt->second.getSchool().getName() != schoolName) {
      return false;
    }

    m_store.students.erase(stIt);
    m_store.roster.erase(
        std::remove_if(
            m_store.roster.begin(), m_store.roster.end(),
            [&](const RosterEntry& entry) { return entry.studentId == sKey; }),
        m_store.roster.end());
    m_store.openTestsByStudent.erase(sKey);
    m_store.assignmentsByStudent.erase(sKey);
    m_store.returnedTestsByStudent.erase(sKey);
    return true;
  }
};

}  // namespace AA::Fake

#endif  // FAKE_SCHOOL_H
