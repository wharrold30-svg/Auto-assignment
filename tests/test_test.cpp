#include "Test.h"

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "Date.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"

// ============================================================================
// 1. QandA CLASS TESTS
// ============================================================================

TEST(QandATest, DefaultAndParameterizedConstructors) {
  // Tests object creation and initialization of question/answer pairs.
  AA::QandA q1;
  EXPECT_EQ(q1.getQuestion(), "");
  EXPECT_EQ(q1.getAnswer(), "");

  AA::QandA q2("What is C++?", "A compiled programming language.");
  EXPECT_EQ(q2.getQuestion(), "What is C++?");
  EXPECT_EQ(q2.getAnswer(), "A compiled programming language.");
}

TEST(QandATest, SettersAndGetters) {
  // Tests mutating and retrieving question/answer data fields.
  AA::QandA q;
  q.setQuestion("Capital of France?");
  q.setAnswer("Paris");

  EXPECT_EQ(q.getQuestion(), "Capital of France?");
  EXPECT_EQ(q.getAnswer(), "Paris");
}

TEST(QandATest, StructuredBindingsAndPairConversion) {
  // Tests operator std::pair conversion and template get<I>() for structured
  // bindings.
  AA::QandA q("2 + 2?", "4");

  std::pair<std::string, std::string> p = q;
  EXPECT_EQ(p.first, "2 + 2?");
  EXPECT_EQ(p.second, "4");

  EXPECT_EQ(q.get<0>(), "2 + 2?");
  EXPECT_EQ(q.get<1>(), "4");

  // Verify C++ structured bindings work correctly
  auto [question, answer] = q;
  EXPECT_EQ(question, "2 + 2?");
  EXPECT_EQ(answer, "4");
}

TEST(QandATest, RelationalAndEqualityOperators) {
  // Tests spaceship operator (<=>) and equality operator (==) on QandA objects.
  AA::QandA q1("Alpha", "Beta");
  AA::QandA q2("Alpha", "Beta");
  AA::QandA q3("Delta", "Gamma");

  EXPECT_TRUE(q1 == q2);
  EXPECT_FALSE(q1 == q3);
  EXPECT_TRUE(q1 < q3);
}

TEST(QandATest, JsonSerialization) {
  // Tests nlohmann/json intrusive serialization and deserialization for QandA.
  AA::QandA q("Test Question?", "Test Answer");
  nlohmann::json j = q;
  AA::QandA q2 = j.get<AA::QandA>();

  EXPECT_EQ(q.getQuestion(), q2.getQuestion());
  EXPECT_EQ(q.getAnswer(), q2.getAnswer());
}

// ============================================================================
// 2. TEST CLASS: Constructors, Metadata Getters & Setters
// ============================================================================

TEST(TestClassTest, ConstructorsAndMetadataGetSet) {
  // Tests Test constructors, level, teacher assignment, subject, description,
  // and date setters/getters.
  AA::School mockSchool;
  AA::Teacher teacher("Doe", "Jane", "T001", "jane.doe@school.edu", mockSchool);

  AA::Test defaultTest;
  EXPECT_EQ(defaultTest.getTestName(), "");
  EXPECT_EQ(defaultTest.getLevel(), 0);

  AA::Test test("Midterm Exam", teacher, 2);
  EXPECT_EQ(test.getTestName(), "Midterm Exam");
  EXPECT_EQ(test.getLevel(), 2);
  EXPECT_EQ(test.getTeacher().getTeacherID(), "T001");

  test.setTestName("Final Exam");
  test.setSubject("Computer Science");
  test.setDescription("Comprehensive final exam");
  test.setLevel(3);

  DT::Date sentDate("2026-11-01 08:00:00");
  DT::Date dueDate("2026-11-15 17:00:00");
  test.setSentDate(sentDate);
  test.setDueDate(dueDate);

  EXPECT_EQ(test.getTestName(), "Final Exam");
  EXPECT_EQ(test.getSubject(), "Computer Science");
  EXPECT_EQ(test.getDescription(), "Comprehensive final exam");
  EXPECT_EQ(test.getLevel(), 3);
  EXPECT_EQ(test.getSentDate().toStringSql(), sentDate.toStringSql());
  EXPECT_EQ(test.getDueDate().toStringSql(), dueDate.toStringSql());
}

// ============================================================================
// 3. TEST CLASS: Student Management (addStudent, removeStudent, hasStudent,
// getStudents)
// ============================================================================

TEST(TestClassTest, StudentManagement) {
  // Tests adding, removing, checking enrollment, and filtering students by
  // class period.
  AA::School mockSchool;
  AA::Teacher teacher("Smith", "John", "T002", "john@school.edu", mockSchool);
  AA::Test test("Quiz 1", teacher, 1);

  auto student1 = std::make_shared<AA::Student>("S01", "s1@school.edu", "Adams",
                                                "Alice", 1, "HighSchool");
  auto student2 = std::make_shared<AA::Student>("S02", "s2@school.edu", "Baker",
                                                "Bob", 2, "HighSchool");

  EXPECT_FALSE(test.hasStudent(student1));

  test.addStudent(student1);
  test.addStudent(student2);

  EXPECT_TRUE(test.hasStudent(student1));
  EXPECT_TRUE(test.hasStudent(student2));

  // Retrieve all students (period 0 = all periods)
  auto allStudentsMap = test.getStudents(0);
  ASSERT_TRUE(allStudentsMap.has_value());
  EXPECT_EQ(allStudentsMap->size(), 2);

  // Retrieve students filtered by period 1
  auto period1Map = test.getStudents(1);
  ASSERT_TRUE(period1Map.has_value());
  EXPECT_EQ(period1Map->size(), 1);
  EXPECT_EQ(period1Map->begin()->first->getStudentID(), "S01");

  // Remove student and verify
  test.removeStudent(student1);
  EXPECT_FALSE(test.hasStudent(student1));
  EXPECT_TRUE(test.hasStudent(student2));
}

// ============================================================================
// 4. TEST CLASS: Student Submissions, Grading, and QandA Operations
// ============================================================================

TEST(TestClassTest, StudentTestSubmissionAndGrading) {
  // Tests setting questions, submitting student test answers, recording
  // submission dates, grading, and identifying wrong answers.
  AA::School mockSchool;
  AA::Teacher teacher("Davis", "Mark", "T003", "mark@school.edu", mockSchool);
  AA::Test test("Math Quiz", teacher, 1);

  AA::TestVec questions = {{"What is 2+2?", "4"}, {"What is 5*5?", "25"}};
  test.setTest(questions);
  EXPECT_EQ(test.getTest().size(), 2);

  auto student = std::make_shared<AA::Student>("S100", "student@school.edu",
                                               "Clark", "Charlie", 1, "School");
  test.addStudent(student);

  EXPECT_FALSE(test.rcvTest(student));

  // Submit test: 1st answer correct ("4"), 2nd answer wrong ("20" instead of
  // "25")
  std::vector<std::string> studentAnswers = {"4", "20"};
  DT::Date subDate("2026-11-10 10:00:00");

  bool submitted = test.setStudentTest(student, studentAnswers, subDate);
  EXPECT_TRUE(submitted);
  EXPECT_TRUE(test.rcvTest(student));

  auto retrievedAnswers = test.getStudentTest(student);
  EXPECT_EQ(retrievedAnswers, studentAnswers);

  auto rcvDate = test.getRcvDate(student);
  ASSERT_TRUE(rcvDate.has_value());
  EXPECT_EQ(rcvDate->toStringSql(), subDate.toStringSql());

  // Execute grading routine
  test.gradeTests();

  // 1 correct out of 2 = 50.0% grade
  float grade = test.getGrade(student);
  EXPECT_FLOAT_EQ(grade, 50.0f);

  // Index 1 should be flagged as wrong
  auto wrongAns = test.getWrongAnswers(student);
  ASSERT_TRUE(wrongAns.has_value());
  ASSERT_EQ(wrongAns->size(), 1);
  EXPECT_EQ((*wrongAns)[0], 1);
}

// ============================================================================
// 5. TEST CLASS: Utilities, Randomization & Operators
// ============================================================================

TEST(TestClassTest, RandomizationAndRelationalOperators) {
  // Tests sorting based on due dates (<=>) and equality checks (==), plus
  // question shuffling. Note: DT::Date considers newer dates "less than" older
  // dates in spaceship comparisons.
  AA::School mockSchool;
  AA::Teacher teacher("Lee", "Ann", "T004", "ann@school.edu", mockSchool);

  AA::Test test1("Math Exam", teacher, 1);
  test1.setDueDate(
      DT::Date("2026-11-20 00:00:00"));  // Newer date (smaller in <=> order)

  AA::Test test2("Science Exam", teacher, 1);
  test2.setDueDate(
      DT::Date("2026-11-10 00:00:00"));  // Older date (larger in <=> order)

  // Spaceship operator sorts tests by due date (newer < older)
  EXPECT_TRUE(test1 < test2);

  AA::Test test1_clone("Math Exam", teacher, 1);
  EXPECT_TRUE(test1 == test1_clone);

  AA::TestVec qVec = {{"Q1", "A1"}, {"Q2", "A2"}, {"Q3", "A3"}};
  test1.setTest(qVec);
  AA::TestVec shuffled = test1.random();
  EXPECT_EQ(shuffled.size(), 3);
}

TEST(TestClassTest, FormattingAndSerialization) {
  // Tests ostream insertion and nlohmann/json persistence for the Test class.
  AA::School mockSchool;
  AA::Teacher teacher("White", "Walter", "T005", "walter@school.edu",
                      mockSchool);
  AA::Test test("Chemistry Quiz", teacher, 2);
  test.setSubject("Chemistry");

  std::stringstream ss;
  ss << test;
  EXPECT_FALSE(ss.str().empty());

  nlohmann::json j = test;
  AA::Test testLoaded = j.get<AA::Test>();
  EXPECT_EQ(test.getTestName(), testLoaded.getTestName());
  EXPECT_EQ(test.getLevel(), testLoaded.getLevel());
}
