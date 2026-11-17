#include <gtest/gtest.h>

#include <cmath>
#include <ctime>
#include <iostream>
#include <string>
#include <utility>

#include "Date.h"
#include "GradeRecord.h"

namespace {

// ANSI color codes matching Google Test's native palette
constexpr const char* COLOR_RESET = "\033[0m";
constexpr const char* COLOR_GREEN = "\033[0;32m";
constexpr const char* COLOR_RED = "\033[0;31m";
constexpr const char* COLOR_CYAN = "\033[0;36m";

// Custom listener that runs ALONGSIDE Google Test's native colored printer
// to announce each part being tested and its pass/fail status.
class PartReporterListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo& test_info) override {
    std::cout << COLOR_CYAN << "[ PART     ] " << COLOR_RESET
              << "Testing: " << test_info.test_suite_name() << "."
              << test_info.name() << "\n";
  }

  void OnTestEnd(const ::testing::TestInfo& test_info) override {
    if (test_info.result()->Passed()) {
      std::cout << COLOR_GREEN << "[ RESULT   ] " << COLOR_RESET
                << test_info.name() << " -> PASSED\n\n";
    } else {
      std::cout << COLOR_RED << "[ RESULT   ] " << COLOR_RESET
                << test_info.name() << " -> FAILED\n\n";
    }
  }
};

// Test Fixture providing reusable baseline data for GradeRecord tests
class GradeRecordTest : public ::testing::Test {
 protected:
  // Constructs DT::Date using explicit Date(const std::tm&)
  DT::Date makeDate(int year, int month, int day, int hour = 12, int min = 0,
                    int sec = 0) {
    std::tm tm_val{};
    tm_val.tm_year = year - 1900;  // std::tm years are since 1900
    tm_val.tm_mon = month - 1;     // std::tm months are 0-11
    tm_val.tm_mday = day;
    tm_val.tm_hour = hour;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;
    tm_val.tm_isdst = 0;
    return DT::Date{tm_val};
  }

  AA::GradeRecord createSampleRecord() {
    return AA::GradeRecord(
        "TEST-101", "Midterm Exam", "Computer Science",
        makeDate(2026, 10, 15),  // Passed as rvalue to match const DT::Date&&
        makeDate(2026, 10, 14), 95, "Graded");
  }
};

// ============================================================================
// 1. Constructor & Getter Tests
// ============================================================================

TEST_F(GradeRecordTest, ParameterizedConstructorAndGetters) {
  SCOPED_TRACE("Testing Parameterized Constructor and all Getters");

  DT::Date expectedDue = makeDate(2026, 10, 15);
  DT::Date expectedSub = makeDate(2026, 10, 14);

  AA::GradeRecord record = createSampleRecord();

  EXPECT_EQ(record.getTestID(), "TEST-101");
  EXPECT_EQ(record.getTestName(), "Midterm Exam");
  EXPECT_EQ(record.getSubject(), "Computer Science");
  EXPECT_EQ(record.getDueDate(), expectedDue);
  EXPECT_EQ(record.getSubmissionDate(), expectedSub);
  EXPECT_EQ(record.getGrade(), 95);
  EXPECT_EQ(record.getStatus(), "Graded");
}

TEST_F(GradeRecordTest, DefaultConstructorInitialization) {
  SCOPED_TRACE("Testing Default Constructor baseline state");

  AA::GradeRecord defaultRecord;
  DT::Date defaultDate;

  EXPECT_TRUE(defaultRecord.getTestID().empty());
  EXPECT_TRUE(defaultRecord.getTestName().empty());
  EXPECT_TRUE(defaultRecord.getSubject().empty());
  EXPECT_TRUE(defaultRecord.getStatus().empty());

  EXPECT_TRUE(defaultRecord.getDueDate().empty());
  EXPECT_TRUE(defaultRecord.getSubmissionDate().empty());
  EXPECT_EQ(defaultRecord.getDueDate(), defaultDate);
  EXPECT_EQ(defaultRecord.getSubmissionDate(), defaultDate);
}

// ============================================================================
// 2. Setter Tests
// ============================================================================

TEST_F(GradeRecordTest, SettersUpdateFieldsAccurately) {
  SCOPED_TRACE("Testing all Setter methods and verifying via Getters");

  AA::GradeRecord record = createSampleRecord();

  DT::Date newDue = makeDate(2026, 11, 20);
  DT::Date newSub = makeDate(2026, 11, 19);

  record.setTestID("TEST-202");
  record.setTestName("Final Exam");
  record.setSubject("Algorithms");
  record.setDueDate(newDue);
  record.setSubmissionDate(newSub);
  record.setGrade(100);
  record.setStatus("Returned");

  EXPECT_EQ(record.getTestID(), "TEST-202");
  EXPECT_EQ(record.getTestName(), "Final Exam");
  EXPECT_EQ(record.getSubject(), "Algorithms");
  EXPECT_EQ(record.getDueDate(), newDue);
  EXPECT_EQ(record.getSubmissionDate(), newSub);
  EXPECT_EQ(record.getGrade(), 100);
  EXPECT_EQ(record.getStatus(), "Returned");
}

TEST_F(GradeRecordTest, GradeBoundaryValues) {
  SCOPED_TRACE(
      "Testing Grade setter with zero, negative, and extra-credit values");

  AA::GradeRecord record = createSampleRecord();

  record.setGrade(0);
  EXPECT_EQ(record.getGrade(), 0);

  record.setGrade(105);
  EXPECT_EQ(record.getGrade(), 105);

  record.setGrade(-1);
  EXPECT_EQ(record.getGrade(), -1);
}

// ============================================================================
// 3. Overloaded Operator Tests (== and <=>)
// ============================================================================

TEST_F(GradeRecordTest, EqualityOperatorComparesOnlyTestID) {
  SCOPED_TRACE("Testing operator== checks m_testID exclusively");

  AA::GradeRecord recordA("ID-999", "Quiz 1", "Math", makeDate(2026, 9, 1),
                          makeDate(2026, 9, 1), 80, "Graded");

  // Same ID, completely different fields
  AA::GradeRecord recordSameID("ID-999", "Final", "History",
                               makeDate(2026, 12, 15), makeDate(2026, 12, 14),
                               100, "Pending");

  // Different ID, identical other fields
  AA::GradeRecord recordDiffID("ID-888", "Quiz 1", "Math", makeDate(2026, 9, 1),
                               makeDate(2026, 9, 1), 80, "Graded");

  EXPECT_TRUE(recordA == recordSameID);
  EXPECT_FALSE(recordA != recordSameID);

  EXPECT_FALSE(recordA == recordDiffID);
  EXPECT_TRUE(recordA != recordDiffID);
}

TEST_F(GradeRecordTest, SpaceshipOperatorComparesDueDate) {
  SCOPED_TRACE(
      "Testing operator<=> orders newer due dates before older due dates");

  AA::GradeRecord olderDue("ID-1", "Test A", "CS", makeDate(2026, 10, 1),
                           makeDate(2026, 10, 1), 90, "Graded");

  AA::GradeRecord newerDue("ID-2", "Test B", "CS", makeDate(2026, 10, 10),
                           makeDate(2026, 10, 5), 70, "Graded");

  AA::GradeRecord sameOlderDueDifferentID("ID-3", "Test C", "Math",
                                          makeDate(2026, 10, 1),
                                          makeDate(2026, 10, 2), 85, "Graded");

  // Per Date.h: newer dates are less than older dates (lhs < rhs when lhs is
  // newer)
  EXPECT_TRUE(newerDue < olderDue);
  EXPECT_TRUE(newerDue <= olderDue);
  EXPECT_TRUE(olderDue > newerDue);
  EXPECT_TRUE(olderDue >= newerDue);

  // Direct three-way ordering checks
  EXPECT_EQ(newerDue <=> olderDue, std::strong_ordering::less);
  EXPECT_EQ(olderDue <=> newerDue, std::strong_ordering::greater);
  EXPECT_EQ(olderDue <=> sameOlderDueDifferentID, std::strong_ordering::equal);

  // Verify nuance: <=> equal due date does NOT imply operator== is true
  EXPECT_FALSE(olderDue == sameOlderDueDifferentID);
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // Append PartReporterListener WITHOUT removing default_result_printer()
  // so Google Test's native colored output and summary remain active.
  ::testing::TestEventListeners& listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new PartReporterListener);

  return RUN_ALL_TESTS();
}
