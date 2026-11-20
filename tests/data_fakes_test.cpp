#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Date.h"
#include "GradeRecord.h"
#include "Gradebook.h"
#include "OpenTest.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "Test.h"
#include "fakes/FakeGradebook.h"
#include "fakes/FakeSchool.h"
#include "fakes/FakeStore.h"
#include "fakes/FakeStudent.h"
#include "fakes/FakeTeacher.h"
#include "fakes/FakeTest.h"
namespace {

class FakesSuiteTest : public ::testing::Test {
 protected:
  AA::Fake::Store store;
  AA::Fake::FakeTeacher teacherFake{store};
  AA::Fake::FakeStudent studentFake{store};
  AA::Fake::FakeSchool schoolFake{store};
  AA::Fake::FakeGradebook gradebookFake{store};
  AA::Fake::FakeTest testFake{store};

  void SetUp() override {
    AA::Gradebook gbA;
    gbA.setName("Canvas");
    gbA.setURL("https://canvas.example.edu");
    gbA.setPostTemp("post_canvas.py");
    store.gradebooks["Canvas"] = gbA;

    AA::Gradebook gbB;
    gbB.setName("PowerSchool");
    gbB.setURL("https://ps.example.edu");
    gbB.setPostTemp("post_ps.py");
    store.gradebooks["PowerSchool"] = gbB;

    AA::School northHigh;
    northHigh.setName("North High");
    northHigh.setAddress("100 North St");
    northHigh.setGradebook("Canvas");
    store.schools["North High"] = northHigh;

    AA::School southHigh;
    southHigh.setName("South High");
    southHigh.setAddress("200 South Ave");
    southHigh.setGradebook("PowerSchool");
    store.schools["South High"] = southHigh;

    AA::Teacher t1;
    t1.setTeacherID("T100");
    t1.setFName("Grace");
    t1.setLName("Hopper");
    t1.setEmailAddr("ghopper@north.edu");
    t1.setSchool(northHigh);
    store.teachers["T100"] = t1;

    AA::Student s1;
    s1.setStudentID("S100");
    s1.setFName("Alan");
    s1.setLName("Turing");
    s1.setEmailAddr("aturing@north.edu");
    s1.setClassPeriod(2);
    s1.setSchool(northHigh);
    store.students["S100"] = s1;

    AA::Student s2;
    s2.setStudentID("S101");
    s2.setFName("Ada");
    s2.setLName("Lovelace");
    s2.setEmailAddr("alovelace@north.edu");
    s2.setClassPeriod(2);
    s2.setSchool(northHigh);
    store.students["S101"] = s2;
  }
};

TEST_F(FakesSuiteTest, LoadsTeachersAndStudentsForSchool) {
  std::vector<AA::Teacher> teachers;
  ASSERT_TRUE(schoolFake.loadTeachersForSchool("North High", teachers));
  ASSERT_EQ(teachers.size(), 1U);
  EXPECT_EQ(teachers[0].getTeacherID(), "T100");

  std::vector<AA::Student> students;
  ASSERT_TRUE(schoolFake.loadStudentsForSchool("North High", students));
  EXPECT_EQ(students.size(), 2U);

  std::vector<AA::Student> emptyStudents;
  ASSERT_TRUE(schoolFake.loadStudentsForSchool("South High", emptyStudents));
  EXPECT_TRUE(emptyStudents.empty());

  EXPECT_FALSE(schoolFake.loadTeachersForSchool("", teachers));
  EXPECT_FALSE(schoolFake.loadStudentsForSchool("", students));
}

TEST_F(FakesSuiteTest, TransfersAndRemovesTeacherFromSchool) {
  ASSERT_TRUE(teacherFake.enrollStudentInClass("T100", "S100", 2));
  ASSERT_TRUE(schoolFake.transferTeacherSchool("T100", "South High"));

  std::vector<AA::Teacher> northTeachers;
  ASSERT_TRUE(schoolFake.loadTeachersForSchool("North High", northTeachers));
  EXPECT_TRUE(northTeachers.empty());

  std::vector<AA::Teacher> southTeachers;
  ASSERT_TRUE(schoolFake.loadTeachersForSchool("South High", southTeachers));
  ASSERT_EQ(southTeachers.size(), 1U);
  EXPECT_EQ(southTeachers[0].getSchool().getName(), "South High");

  ASSERT_TRUE(schoolFake.removeTeacherFromSchool("T100"));
  EXPECT_TRUE(store.teachers["T100"].getSchool().getName().empty());

  std::vector<std::pair<std::string, unsigned int>> roster;
  ASSERT_TRUE(teacherFake.loadTeacherRoster("T100", roster));
  EXPECT_TRUE(roster.empty());
}

TEST_F(FakesSuiteTest, TransfersAndRemovesStudentCompletely) {
  ASSERT_TRUE(teacherFake.enrollStudentInClass("T100", "S100", 2));
  ASSERT_TRUE(schoolFake.transferStudentSchool("S100", "South High"));
  EXPECT_EQ(store.students["S100"].getSchool().getName(), "South High");

  // Fails when schoolName does not match the student's current school.
  EXPECT_FALSE(
      schoolFake.removeStudentFromSchoolCompletely("S100", "North High"));
  ASSERT_TRUE(
      schoolFake.removeStudentFromSchoolCompletely("S100", "South High"));

  EXPECT_EQ(store.students.find("S100"), store.students.end());
  std::vector<std::tuple<std::string, unsigned int>> schedule;
  ASSERT_TRUE(studentFake.loadStudentSchedule("S100", schedule));
  EXPECT_TRUE(schedule.empty());
}

TEST_F(FakesSuiteTest, ManagesRosterAndCredentials) {
  EXPECT_TRUE(teacherFake.saveCredentials("T100", "salt123", "hash456"));
  std::string salt;
  std::string password;
  ASSERT_TRUE(teacherFake.loadCredentials("T100", salt, password));
  EXPECT_EQ(salt, "salt123");
  EXPECT_EQ(password, "hash456");

  const std::vector<std::pair<std::string, unsigned int>> pairs = {{"S100", 2},
                                                                   {"S101", 4}};
  ASSERT_TRUE(teacherFake.saveTeacherRoster("T100", pairs));

  std::vector<std::string> period2Ids;
  ASSERT_TRUE(teacherFake.loadRosterForPeriod("T100", 2, period2Ids));
  ASSERT_EQ(period2Ids.size(), 1U);
  EXPECT_EQ(period2Ids[0], "S100");

  ASSERT_TRUE(teacherFake.removeStudentFromClass("T100", "S100", 2));
  std::vector<std::tuple<std::string, unsigned int>> s100Schedule;
  ASSERT_TRUE(studentFake.loadStudentSchedule("S100", s100Schedule));
  EXPECT_TRUE(s100Schedule.empty());

  AA::Teacher foundTeacher;
  ASSERT_TRUE(
      teacherFake.loadTeacherByEmail("ghopper@north.edu", foundTeacher));
  EXPECT_EQ(foundTeacher.getTeacherID(), "T100");

  AA::Student foundStudent;
  ASSERT_TRUE(
      studentFake.loadStudentByEmail("alovelace@north.edu", foundStudent));
  EXPECT_EQ(foundStudent.getStudentID(), "S101");
}

TEST_F(FakesSuiteTest, AssignsAndReturnsStudentTestSeparatelyFromGradeRecord) {
  AA::Teacher quizTeacher;
  quizTeacher.setTeacherID("T100");

  AA::Test quiz;
  quiz.setTestID("TEST_01");
  quiz.setTeacher(quizTeacher);
  quiz.setTestName("Logic Quiz");

  AA::QandA q1;
  q1.setQuestion("2 + 2?");
  q1.setAnswer("4");
  store.tests["TEST_01"] = quiz;
  store.testQuestions["TEST_01"] = {q1};

  AA::Test loadedQuiz;
  ASSERT_TRUE(testFake.loadTestWithQuestions("TEST_01", loadedQuiz));
  EXPECT_EQ(loadedQuiz.getTestID(), "TEST_01");
  ASSERT_EQ(loadedQuiz.getTest().size(), 1U);
  EXPECT_EQ(loadedQuiz.getTest()[0].getAnswer(), "4");

  std::vector<AA::Test> teacherTests;
  ASSERT_TRUE(teacherFake.loadTestsForTeacher("T100", teacherTests));
  ASSERT_EQ(teacherTests.size(), 1U);
  EXPECT_EQ(teacherTests[0].getTestID(), "TEST_01");

  AA::OpenTest ot;
  ot.setID(42);
  ot.setTestID("TEST_01");
  ot.setTeacherID("T100");
  ot.setCreationDate(DT::Date("2026-11-18 09:00:00"));
  ot.setDueDate(DT::Date("2026-11-25 23:59:00"));
  ot.setDuration(60);
  ot.setStatus("Open");
  store.openTestsByTeacher["T100"].push_back(ot);

  std::vector<AA::OpenTest> teacherOpenTests;
  ASSERT_TRUE(teacherFake.loadOpenTestsForTeacher("T100", teacherOpenTests));
  ASSERT_EQ(teacherOpenTests.size(), 1U);
  EXPECT_EQ(teacherOpenTests[0].getID(), 42);

  ASSERT_TRUE(
      teacherFake.assignOpenTestToMultipleStudents({"S100", "S101"}, 42));

  std::vector<AA::OpenTest> s100OpenTests;
  ASSERT_TRUE(studentFake.loadOpenTestsForStudent("S100", s100OpenTests));
  ASSERT_EQ(s100OpenTests.size(), 1U);
  EXPECT_EQ(s100OpenTests[0].getID(), 42);

  AA::Test submittedTest = quiz;
  submittedTest.setTestID("TEST_01");

  const DT::Date submittedDate("2026-11-18 14:30:00");
  ASSERT_TRUE(studentFake.returnStudentTest(
      "S100", 42, submittedDate, "Submitted", submittedTest, "Q1", 90.0f));

  std::vector<AA::GradeRecord> records;
  ASSERT_TRUE(studentFake.getStudentAssignments("S100", records));
  ASSERT_EQ(records.size(), 1U);
  EXPECT_EQ(records[0].getTestID(), "TEST_01");
  EXPECT_EQ(records[0].getOpenTestID(), 42);
  EXPECT_FLOAT_EQ(records[0].getGrade(), 90.0f);
  EXPECT_EQ(records[0].getStatus(), "Submitted");

  std::vector<AA::Test> returnedTests;
  ASSERT_TRUE(studentFake.getReturnedTestsForStudent("S100", returnedTests));
  ASSERT_EQ(returnedTests.size(), 1U);
  EXPECT_EQ(returnedTests[0].getTestID(), "TEST_01");

  ASSERT_TRUE(testFake.removeTestQuestions("TEST_01"));
  std::vector<AA::QandA> remainingQuestions;
  ASSERT_TRUE(testFake.loadTestQuestions("TEST_01", remainingQuestions));
  EXPECT_TRUE(remainingQuestions.empty());
}

TEST_F(FakesSuiteTest, LoadsAndUpdatesGradebooksForSchool) {
  AA::Gradebook loaded;
  ASSERT_TRUE(gradebookFake.loadGradeBookForSchool("North High", loaded));
  EXPECT_EQ(loaded.getName(), "Canvas");

  ASSERT_TRUE(gradebookFake.updateGradeBookForSchool(
      "North High", store.gradebooks["PowerSchool"]));
  ASSERT_TRUE(gradebookFake.loadGradeBookForSchool("North High", loaded));
  EXPECT_EQ(loaded.getName(), "PowerSchool");

  std::vector<std::pair<std::string, AA::Gradebook>> allGradebooks;
  ASSERT_TRUE(gradebookFake.loadAllGradeBooks(allGradebooks));
  ASSERT_EQ(allGradebooks.size(), 2U);
}

}  // namespace
