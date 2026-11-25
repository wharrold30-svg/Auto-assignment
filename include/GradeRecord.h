#ifndef GRADE_RECORD_H
#define GRADE_RECORD_H

#include <compare>
#include <string>

#include "Date.h"
#include "nlohmann/json.hpp"
namespace AA {
class GradeRecord {
 private:
  std::string m_studentID;
  std::string m_testID;
  ;
  int m_openTestID;
  std::string m_testName;
  std::string m_subject;
  DT::Date m_dueDate;
  DT::Date m_submissionDate;
  float m_grade;
  std::string m_status;

 public:
  // Constructors
  GradeRecord() = default;
  GradeRecord(const std::string& studentId, const std::string& testID,
              const int openTestID, const std::string& testName,
              const std::string& subject, const DT::Date& dueDate,
              const DT::Date& submissionDate, float grade,
              const std::string& status)
      : m_studentID(studentId),
        m_testID(testID),
        m_openTestID(openTestID),
        m_testName(testName),
        m_subject(subject),
        m_dueDate(dueDate),
        m_submissionDate(submissionDate),
        m_grade(grade),
        m_status(status) {}

  // Getters
  const std::string& getStudentID() const { return m_studentID; }

  const std::string& getTestID() const { return m_testID; }

  const std::string& getTestName() const { return m_testName; }

  int getOpenTestID() const { return m_openTestID; }

  const std::string& getSubject() const { return m_subject; }

  const DT::Date& getDueDate() const { return m_dueDate; }

  const DT::Date& getSubmissionDate() const { return m_submissionDate; }

  float getGrade() const { return m_grade; }

  const std::string& getStatus() const { return m_status; }

  // Setters
  void setStudentID(const std::string& studentID) { m_studentID = studentID; }

  void setTestID(const std::string& testID) { m_testID = testID; }

  void setOpenTestID(const int id) { m_openTestID = id; }

  void setTestName(const std::string& testName) { m_testName = testName; }

  void setSubject(const std::string& subject) { m_subject = subject; }

  void setDueDate(const DT::Date& dueDate) { m_dueDate = dueDate; }

  void setSubmissionDate(const DT::Date& submissionDate) {
    m_submissionDate = submissionDate;
  }

  void setGrade(float grade) { m_grade = grade; }

  void setStatus(const std::string& status) { m_status = status; }

  // --- Overloaded Operators ---

  // Spaceship operator: Compares records based on due date
  std::strong_ordering operator<=>(const GradeRecord& other) const {
    return m_dueDate <=> other.m_dueDate;
  }

  // Equality operator: Checks equality based on the test ID
  bool operator==(const GradeRecord& other) const {
    return m_studentID == other.m_studentID;
  }
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(GradeRecord, m_openTestID, m_grade, m_status,
                                 m_subject, m_testID, m_testName, m_studentID,
                                 m_dueDate, m_submissionDate)
};  // GradeRecord
}  // namespace AA
#endif  // GRADE_RECORD_H
