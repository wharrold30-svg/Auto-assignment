#ifndef FAKE_STUDENT_H
#define FAKE_STUDENT_H

#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Date.h"
#include "FakeStore.h"
#include "GradeRecord.h"
#include "OpenTest.h"
#include "Student.h"
#include "Test.h"

namespace AA::Fake {

class FakeStudent {
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
  explicit FakeStudent(Store& store) : m_store(store) {}

  bool loadStudentByEmail(std::string_view email,
                          AA::Student& out_student) const {
    for (const auto& [id, student] : m_store.students) {
      if (student.getEmailAddr() == email) {
        out_student = student;
        return true;
      }
    }
    return false;
  }

  bool loadStudentSchedule(
      std::string_view studentId,
      std::vector<std::tuple<std::string, unsigned int>>& out_schedule) const {
    out_schedule.clear();
    const std::string sKey(studentId);
    for (const auto& entry : m_store.roster) {
      if (entry.studentId == sKey) {
        out_schedule.emplace_back(entry.teacherId, entry.classPeriod);
      }
    }
    return true;
  }

  bool loadOpenTestsForStudent(std::string_view studentId,
                               std::vector<AA::OpenTest>& out_openTests) const {
    out_openTests.clear();
    const auto it = m_store.openTestsByStudent.find(std::string(studentId));
    if (it != m_store.openTestsByStudent.end()) {
      out_openTests = it->second;
    }
    return true;
  }

  bool returnStudentTest(std::string_view studentId, int openTestId,
                         const DT::Date& submissionDate,
                         std::string_view status, const AA::Test& submittedTest,
                         std::string_view wrongAnswersSummary, float grade) {
    if (studentId.empty() || openTestId <= 0) {
      return false;
    }

    const std::string sKey(studentId);

    AA::OpenTest matchedOpenTest;
    const bool hasOpenTest = findOpenTestById(openTestId, matchedOpenTest);

    std::string resolvedTestId = submittedTest.getTestID();
    if (resolvedTestId.empty() && hasOpenTest) {
      resolvedTestId = matchedOpenTest.getTestID();
    }

    std::string testName = submittedTest.getTestName();
    std::string subject = submittedTest.getSubject();
    DT::Date dueDate = submittedTest.getDueDate();
    if (hasOpenTest) {
      if (dueDate.empty()) {
        dueDate = matchedOpenTest.getDueDate();
      }
      if (testName.empty()) {
        const auto tIt = m_store.tests.find(resolvedTestId);
        if (tIt != m_store.tests.end()) {
          testName = tIt->second.getTestName();
          if (subject.empty()) {
            subject = tIt->second.getSubject();
          }
        }
      }
    }

    AA::GradeRecord record(sKey, resolvedTestId, openTestId, testName, subject,
                           dueDate, submissionDate, grade, std::string(status));
    m_store.assignmentsByStudent[sKey].push_back(std::move(record));

    AA::Test returnedCopy = submittedTest;
    if (returnedCopy.getTestID().empty() && !resolvedTestId.empty()) {
      returnedCopy.setTestID(resolvedTestId);
    }

    ReturnedTestEntry entry;
    entry.openTestId = openTestId;
    entry.testId = returnedCopy.getTestID();
    entry.wrongAnswers = std::string(wrongAnswersSummary);
    entry.returnedTest = std::move(returnedCopy);
    m_store.returnedTestsByStudent[sKey].push_back(std::move(entry));

    return true;
  }

  bool getStudentAssignments(std::string_view studentId,
                             std::vector<AA::GradeRecord>& out_records) const {
    out_records.clear();
    const auto it = m_store.assignmentsByStudent.find(std::string(studentId));
    if (it != m_store.assignmentsByStudent.end()) {
      out_records = it->second;
    }
    return true;
  }

  bool getReturnedTestsForStudent(std::string_view studentId,
                                  std::vector<AA::Test>& out_tests) const {
    out_tests.clear();
    const auto it = m_store.returnedTestsByStudent.find(std::string(studentId));
    if (it != m_store.returnedTestsByStudent.end()) {
      out_tests.reserve(it->second.size());
      for (const auto& entry : it->second) {
        out_tests.push_back(entry.returnedTest);
      }
    }
    return true;
  }
};

}  // namespace AA::Fake

#endif  // FAKE_STUDENT_H
