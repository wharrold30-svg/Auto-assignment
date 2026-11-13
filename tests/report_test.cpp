#include "Report.h"

#include <gtest/gtest.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "Date.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "Test.h"

// Helper fixture to set up a mock Test with unique student names for Report
// testing
class ReportTestFixture : public ::testing::Test {
 protected:
  AA::School mockSchool;
  AA::Teacher teacher{"Smith", "John", "T001", "john.smith@school.edu",
                      mockSchool};
  AA::Test testObj{"Final Physics Exam", teacher, 2};

  std::shared_ptr<AA::Student> s1;
  std::shared_ptr<AA::Student> s2;
  std::shared_ptr<AA::Student> s3;
  std::shared_ptr<AA::Student> s4;
  std::shared_ptr<AA::Student> s5;
  std::shared_ptr<AA::Student> s6;

  void SetUp() override {
    // Setup test questions
    AA::TestVec questions = {
        {"Q1", "A1"}, {"Q2", "A2"}, {"Q3", "A3"}, {"Q4", "A4"}};
    testObj.setTest(questions);

    // Create students with completely unique last/first names to avoid std::map
    // key collisions
    s1 = std::make_shared<AA::Student>("S01", "s1@school.edu", "Alpha", "Alice",
                                       1, "HighSchool");
    s2 = std::make_shared<AA::Student>("S02", "s2@school.edu", "Bravo", "Bob",
                                       1, "HighSchool");
    s3 = std::make_shared<AA::Student>("S03", "s3@school.edu", "Charlie",
                                       "Cathy", 2, "HighSchool");
    s4 = std::make_shared<AA::Student>("S04", "s4@school.edu", "Delta", "David",
                                       2, "HighSchool");
    s5 = std::make_shared<AA::Student>("S05", "s5@school.edu", "Echo", "Emma",
                                       3, "HighSchool");
    s6 = std::make_shared<AA::Student>("S06", "s6@school.edu", "Foxtrot",
                                       "Frank", 3, "HighSchool");

    testObj.addStudent(s1);
    testObj.addStudent(s2);
    testObj.addStudent(s3);
    testObj.addStudent(s4);
    testObj.addStudent(s5);
    testObj.addStudent(s6);

    DT::Date subDate("2026-11-12 10:00:00");

    // Mock submissions & grades: 100, 75, 50, 100, 25, 75 -> Sum = 425 -> Mean
    // = 70.8333...
    testObj.setStudentTest(s1, {"A1", "A2", "A3", "A4"}, subDate);     // 100.0%
    testObj.setStudentTest(s2, {"A1", "A2", "A3", "Wrong"}, subDate);  // 75.0%
    testObj.setStudentTest(s3, {"A1", "A2", "Wrong", "Wrong"},
                           subDate);                                // 50.0%
    testObj.setStudentTest(s4, {"A1", "A2", "A3", "A4"}, subDate);  // 100.0%
    testObj.setStudentTest(s5, {"A1", "Wrong", "Wrong", "Wrong"},
                           subDate);                                   // 25.0%
    testObj.setStudentTest(s6, {"A1", "A2", "A3", "Wrong"}, subDate);  // 75.0%

    testObj.gradeTests();
  }
};

// ============================================================================
// 1. REPORT CLASS: Construction & Basic Statistics (Mean, Median, Var, SD)
// ============================================================================

TEST_F(ReportTestFixture, DefaultAndParameterizedConstruction) {
  // Tests Report default constructor and initialization from a graded Test
  // object.
  AA::Report defaultReport;

  AA::Report report(testObj);
  // Total sum = 425. Mean = 425 / 6 = 70.8333...
  EXPECT_NEAR(report.mean(), 70.8333f, 0.01f);
}

TEST_F(ReportTestFixture, StatisticalCalculations) {
  // Tests statistical evaluation methods: mean(), meadian() [median], var(),
  // and sd().
  AA::Report report(testObj);

  // Grades sorted: 25, 50, 75, 75, 100, 100 -> Median = (75 + 75) / 2 = 75.0
  EXPECT_FLOAT_EQ(report.meadian(), 75.0f);
  EXPECT_NEAR(report.mean(), 70.8333f, 0.01f);

  // Variance and Standard Deviation checks
  EXPECT_GE(report.var(), 0.0f);
  EXPECT_GE(report.sd(), 0.0f);
}

// ============================================================================
// 2. REPORT CLASS: Rankings (Top Five & Bottom Five)
// ============================================================================

TEST_F(ReportTestFixture, TopAndBottomRankings) {
  // Tests topFive() and bottomFive() ranking multimaps returned in descending
  // order.
  AA::Report report(testObj);

  auto top = report.topFive();
  EXPECT_FALSE(top.empty());
  EXPECT_FLOAT_EQ(top.begin()->first, 100.0f);

  auto bottom = report.bottomFive();
  EXPECT_FALSE(bottom.empty());
  float lowestVal = bottom.rbegin()->first;
  EXPECT_FLOAT_EQ(lowestVal, 25.0f);
}

// ============================================================================
// 3. REPORT CLASS: Percentage Filters (Pass / Fail)
// ============================================================================

TEST_F(ReportTestFixture, PassAndFailPercentages) {
  // Tests passPercentage() and failPercentage() calculations based on standard
  // cutoffs.
  AA::Report report(testObj);

  float passPct = report.passPercentage();
  float failPct = report.failPercentage();

  EXPECT_GE(passPct, 0.0f);
  EXPECT_LE(passPct, 100.0f);
  EXPECT_GE(failPct, 0.0f);
  EXPECT_LE(failPct, 100.0f);
  EXPECT_FLOAT_EQ(passPct + failPct, 100.0f);
}

// ============================================================================
// 4. REPORT UTILITIES & REPORT GENERATORS
// ============================================================================

TEST_F(ReportTestFixture, ReportStringBuildersAndFormatting) {
  // Tests buildTeacherReport, buildStundentReport, maxNameLength, and ostream
  // operator<<.
  std::string teacherRep = AA::buildTeacherReport(testObj);
  EXPECT_FALSE(teacherRep.empty());

  std::string studentRep = AA::buildStundentReport(s1, testObj);
  EXPECT_FALSE(studentRep.empty());

  AA::Report report(testObj);
  AA::GradeMap sampleMap = report.topFive();
  std::size_t maxLen = AA::maxNameLength(sampleMap);
  EXPECT_GE(maxLen, 0u);

  std::stringstream ss;
  ss << report;
  EXPECT_FALSE(ss.str().empty());
}
