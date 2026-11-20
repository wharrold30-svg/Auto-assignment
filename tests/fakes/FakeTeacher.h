#ifndef FAKE_TEACHER_H
#define FAKE_TEACHER_H

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Date.h"
#include "FakeStore.h"
#include "OpenTest.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "Test.h"

namespace AA::Fake {

class FakeTeacher {
 private:
  Store& m_store;

  bool findOpenTestById(int openTestId, AA::OpenTest& out_openTest) const {
    for (const auto& [teacherId, openTests] : m_store.openTestsByTeacher) {
      for (const auto& ot : openTests) {
        if (ot.getID() == openTestId) {
          out_openTest = ot;
          return true;
        }
      }
    }
    return false;
  }

 public:
  explicit FakeTeacher(Store& store) : m_store(store) {}

  bool saveCredentials(std::string_view teacherId, std::string_view salt,
                       std::string_view passwordHash) {
    if (teacherId.empty()) {
      return false;
    }
    m_store.credentials[std::string(teacherId)] =
        Credentials{std::string(salt), std::string(passwordHash)};
    return true;
  }

  bool loadCredentials(std::string_view teacherId, std::string& out_salt,
                       std::string& out_passwordHash) const {
    const auto it = m_store.credentials.find(std::string(teacherId));
    if (it == m_store.credentials.end()) {
      return false;
    }
    out_salt = it->second.salt;
    out_passwordHash = it->second.password;
    return true;
  }

  bool loadTeacherByEmail(std::string_view email,
                          AA::Teacher& out_teacher) const {
    for (const auto& [id, teacher] : m_store.teachers) {
      if (teacher.getEmailAddr() == email) {
        out_teacher = teacher;
        return true;
      }
    }
    return false;
  }

  bool enrollStudentInClass(std::string_view teacherId,
                            std::string_view studentId, unsigned int period) {
    if (teacherId.empty() || studentId.empty()) {
      return false;
    }
    const std::string tKey(teacherId);
    const std::string sKey(studentId);
    const auto exists = std::any_of(
        m_store.roster.begin(), m_store.roster.end(),
        [&](const RosterEntry& entry) {
          return entry.teacherId == tKey && entry.studentId == sKey &&
                 entry.classPeriod == period;
        });
    if (!exists) {
      m_store.roster.push_back(RosterEntry{tKey, sKey, period});
    }
    return true;
  }

  bool removeStudentFromClass(std::string_view teacherId,
                              std::string_view studentId, unsigned int period) {
    const std::string tKey(teacherId);
    const std::string sKey(studentId);
    m_store.roster.erase(
        std::remove_if(m_store.roster.begin(), m_store.roster.end(),
                       [&](const RosterEntry& entry) {
                         return entry.teacherId == tKey &&
                                entry.studentId == sKey &&
                                entry.classPeriod == period;
                       }),
        m_store.roster.end());
    return true;
  }

  bool saveTeacherRoster(
      std::string_view teacherId,
      const std::vector<std::pair<std::string, unsigned int>>& roster) {
    if (teacherId.empty()) {
      return false;
    }
    const std::string tKey(teacherId);
    m_store.roster.erase(
        std::remove_if(
            m_store.roster.begin(), m_store.roster.end(),
            [&](const RosterEntry& entry) { return entry.teacherId == tKey; }),
        m_store.roster.end());

    for (const auto& [studentId, period] : roster) {
      m_store.roster.push_back(RosterEntry{tKey, studentId, period});
    }
    return true;
  }

  bool loadTeacherRoster(
      std::string_view teacherId,
      std::vector<std::pair<std::string, unsigned int>>& out_roster) const {
    out_roster.clear();
    const std::string tKey(teacherId);
    for (const auto& entry : m_store.roster) {
      if (entry.teacherId == tKey) {
        out_roster.emplace_back(entry.studentId, entry.classPeriod);
      }
    }
    return true;
  }

  bool loadRosterForPeriod(std::string_view teacherId, unsigned int period,
                           std::vector<std::string>& out_studentIds) const {
    out_studentIds.clear();
    const std::string tKey(teacherId);
    for (const auto& entry : m_store.roster) {
      if (entry.teacherId == tKey && entry.classPeriod == period) {
        out_studentIds.push_back(entry.studentId);
      }
    }
    return true;
  }

  bool loadTestsForTeacher(std::string_view teacherId,
                           std::vector<AA::Test>& out_tests) const {
    out_tests.clear();
    for (const auto& [testId, test] : m_store.tests) {
      if (test.getTeacher().getTeacherID() == teacherId) {
        AA::Test hydrated = test;
        const auto qIt = m_store.testQuestions.find(testId);
        if (qIt != m_store.testQuestions.end()) {
          hydrated.setTest(qIt->second);
        }
        out_tests.push_back(std::move(hydrated));
      }
    }
    return true;
  }

  bool loadOpenTestsForTeacher(std::string_view teacherId,
                               std::vector<AA::OpenTest>& out_openTests) const {
    out_openTests.clear();
    const auto it = m_store.openTestsByTeacher.find(std::string(teacherId));
    if (it != m_store.openTestsByTeacher.end()) {
      for (const auto& ot : it->second) {
        if (ot.getTeacherID() == teacherId || ot.getTeacherID().empty()) {
          out_openTests.push_back(ot);
        }
      }
    }
    return true;
  }

  bool assignOpenTestToStudent(std::string_view studentId, int openTestId) {
    if (studentId.empty() || openTestId <= 0) {
      return false;
    }
    AA::OpenTest ot;
    if (!findOpenTestById(openTestId, ot)) {
      return false;
    }
    auto& studentOpenTests = m_store.openTestsByStudent[std::string(studentId)];
    const auto exists =
        std::any_of(studentOpenTests.begin(), studentOpenTests.end(),
                    [openTestId](const AA::OpenTest& existing) {
                      return existing.getID() == openTestId;
                    });
    if (!exists) {
      studentOpenTests.push_back(std::move(ot));
    }
    return true;
  }

  bool assignOpenTestToMultipleStudents(
      const std::vector<std::string>& studentIds, int openTestId) {
    if (openTestId <= 0) {
      return false;
    }
    for (const auto& studentId : studentIds) {
      if (!assignOpenTestToStudent(studentId, openTestId)) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace AA::Fake

#endif  // FAKE_TEACHER_H
