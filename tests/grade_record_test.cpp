#include <gtest/gtest.h>

#include <string>

#include "Date.h"
#include "GradeRecord.h"

namespace {

TEST(GradeRecordTest, DefaultConstructionAndSetters) {
  AA::GradeRecord record;
  record.setStudentID("S100");
  record.setTestID("T01");
  record.setOpenTestID(42);
  record.setTestName("Midterm Exam");
  record.setSubject("Computer Science");

  const DT::Date dueDate("2026-11-15 23:59:00");
  const DT::Date submissionDate("2026-11-14 10:30:00");
  record.setDueDate(dueDate);
  record.setSubmissionDate(submissionDate);
  record.setGrade(95);
  record.setStatus("Graded");

  EXPECT_EQ(record.getStudentID(), "S100");
  EXPECT_EQ(record.getTestID(), "T01");
  EXPECT_EQ(record.getOpenTestID(), 42);
  EXPECT_EQ(record.getTestName(), "Midterm Exam");
  EXPECT_EQ(record.getSubject(), "Computer Science");
  EXPECT_EQ(record.getDueDate(), dueDate);
  EXPECT_EQ(record.getSubmissionDate(), submissionDate);
  EXPECT_EQ(record.getGrade(), 95);
  EXPECT_EQ(record.getStatus(), "Graded");
}

TEST(GradeRecordTest, ParameterizedConstructorInitializesAllFields) {
  const DT::Date dueDate("2026-12-01 18:00:00");
  const DT::Date submissionDate("2026-11-30 14:15:00");

  AA::GradeRecord record("S200", "TEST_99", 7, "Final Quiz", "Mathematics",
                         dueDate, submissionDate, 88, "Submitted");

  EXPECT_EQ(record.getStudentID(), "S200");
  EXPECT_EQ(record.getTestID(), "TEST_99");
  EXPECT_EQ(record.getOpenTestID(), 7);
  EXPECT_EQ(record.getTestName(), "Final Quiz");
  EXPECT_EQ(record.getSubject(), "Mathematics");
  EXPECT_EQ(record.getDueDate(), dueDate);
  EXPECT_EQ(record.getSubmissionDate(), submissionDate);
  EXPECT_EQ(record.getGrade(), 88);
  EXPECT_EQ(record.getStatus(), "Submitted");
}

TEST(GradeRecordTest, EqualityComparesStudentId) {
  const DT::Date dueA("2026-10-01 00:00:00");
  const DT::Date dueB("2026-11-01 00:00:00");
  const DT::Date subA("2026-09-25 12:00:00");
  const DT::Date subB("2026-10-28 12:00:00");

  AA::GradeRecord first("S100", "T1", 1, "Quiz 1", "Science", dueA, subA, 80,
                        "Open");
  AA::GradeRecord sameStudent("S100", "T2", 2, "Quiz 2", "History", dueB, subB,
                              100, "Graded");
  AA::GradeRecord differentStudent("S101", "T1", 1, "Quiz 1", "Science", dueA,
                                   subA, 80, "Open");

  EXPECT_EQ(first, sameStudent);
  EXPECT_NE(first, differentStudent);
}

TEST(GradeRecordTest, SpaceshipOperatorFollowsDueDateAndDateOrdering) {
  // DT::Date::operator<=> orders newer dates as less than older dates,
  // and GradeRecord::operator<=> delegates directly to m_dueDate <=>
  // other.m_dueDate.
  const DT::Date earlierDue("2026-10-01 00:00:00");
  const DT::Date laterDue("2026-11-01 00:00:00");
  const DT::Date subDate("2026-09-25 12:00:00");

  AA::GradeRecord earlierDueRecord("S100", "T1", 1, "Lab 1", "CS", earlierDue,
                                   subDate, 90, "Graded");
  AA::GradeRecord laterDueRecord("S100", "T2", 2, "Lab 2", "CS", laterDue,
                                 subDate, 92, "Pending");

  EXPECT_GT(earlierDueRecord, laterDueRecord);
  EXPECT_LT(laterDueRecord, earlierDueRecord);
  EXPECT_GE(earlierDueRecord, laterDueRecord);
  EXPECT_LE(laterDueRecord, earlierDueRecord);
}

}  // namespace
