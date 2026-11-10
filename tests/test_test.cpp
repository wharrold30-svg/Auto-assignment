#include "Test.h"

#include <gtest/gtest.h>

#include <sstream>

#include "Date.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"

// ==========================================
// Test Suite for AA::QandA (Inside Test.h)
// ==========================================
TEST(QandATest, ConstructorsAndGettersSetters) {
  std::cout
      << "[TEST] Testing AA::QandA constructors, getters, and setters...\n";

  // Default constructor
  AA::QandA qaDefault;
  EXPECT_EQ(qaDefault.getQuestion(), "");
  EXPECT_EQ(qaDefault.getAnswer(), "");

  // Parameterized constructor
  AA::QandA qa("What is the capital of France?", "Paris");
  EXPECT_EQ(qa.getQuestion(), "What is the capital of France?");
  EXPECT_EQ(qa.getAnswer(), "Paris");

  // Setters
  qa.setQuestion("What is 5 + 5?");
  qa.setAnswer("10");
  EXPECT_EQ(qa.getQuestion(), "What is 5 + 5?");
  EXPECT_EQ(qa.getAnswer(), "10");

  std::cout << "[PASS] AA::QandA constructors, getters, and setters passed.\n";
}

TEST(QandATest, ComparisonAndStructuredBindings) {
  std::cout << "[TEST] Testing AA::QandA comparison, pair conversion, and "
               "structured bindings...\n";

  AA::QandA qa1("Q1", "A1");
  AA::QandA qa2("Q1", "A1");
  AA::QandA qa3("Q2", "A2");

  // Comparison operators
  EXPECT_TRUE(qa1 == qa2);
  EXPECT_FALSE(qa1 == qa3);
  EXPECT_TRUE((qa1 <=> qa3) < 0);

  // Test std::pair conversion
  std::pair<std::string, std::string> p = qa1;
  EXPECT_EQ(p.first, "Q1");
  EXPECT_EQ(p.second, "A1");

  // Test structured bindings
  auto [q, a] = qa1;
  EXPECT_EQ(q, "Q1");
  EXPECT_EQ(a, "A1");

  std::cout << "[PASS] AA::QandA comparison, pair conversion, and structured "
               "bindings passed.\n";
}

TEST(QandATest, JsonSerialization) {
  std::cout << "[TEST] Testing AA::QandA JSON serialization...\n";

  AA::QandA qa("JSON Question", "JSON Answer");
  nlohmann::json j = qa;

  AA::QandA qaFromJson = j.get<AA::QandA>();
  EXPECT_EQ(qaFromJson.getQuestion(), "JSON Question");
  EXPECT_EQ(qaFromJson.getAnswer(), "JSON Answer");

  std::cout << "[PASS] AA::QandA JSON serialization passed.\n";
}

// ==========================================
// Test Suite for AA::Test
// ==========================================
TEST(TestClassTest, ConstructorsGettersAndSetters) {
  std::cout << "[TEST] Testing AA::Test basic constructors, getters, and "
               "setters...\n";

  AA::School school("State High", "456 College Ave", "GradebookSYS");
  AA::Teacher teacher("Smith", "Jane", "T500", "jsmith@school.com", school);

  AA::Test test("Algebra Final", teacher, 11);

  EXPECT_EQ(test.getTestName(), "Algebra Final");
  EXPECT_EQ(test.getLevel(), 11);
  EXPECT_EQ(test.getTeacher().getTeacherID(), "T500");

  // Setters
  test.setTestName("Advanced Algebra");
  test.setLevel(12);
  test.setSubject("Mathematics");
  test.setDescription("Cumulative final exam");

  EXPECT_EQ(test.getTestName(), "Advanced Algebra");
  EXPECT_EQ(test.getLevel(), 12);
  EXPECT_EQ(test.getSubject(), "Mathematics");
  EXPECT_EQ(test.getDescription(), "Cumulative final exam");

  std::cout
      << "[PASS] AA::Test basic constructors, getters, and setters passed.\n";
}

TEST(TestClassTest, StudentManagement) {
  std::cout << "[TEST] Testing AA::Test student roster additions, removals, "
               "and existence checks...\n";

  AA::School school("State High", "456 College Ave", "GradebookSYS");
  AA::Teacher teacher("Smith", "Jane", "T500", "jsmith@school.com", school);
  AA::Test test("Physics Test", teacher, 10);

  auto student = std::make_shared<AA::Student>("S100", "student@school.com",
                                               "Doe", "John", 3, "State High");

  // Initially student should not be in the test roster
  EXPECT_FALSE(test.hasStudent(student));

  // Add student
  test.addStudent(student);
  EXPECT_TRUE(test.hasStudent(student));

  // Remove student
  test.removeStudent(student);
  EXPECT_FALSE(test.hasStudent(student));

  std::cout << "[PASS] AA::Test student management passed.\n";
}

TEST(TestClassTest, TestVectorAndRandom) {
  std::cout << "[TEST] Testing AA::Test question vector assignment and random "
               "shuffling...\n";

  AA::School school("State High", "456 College Ave", "GradebookSYS");
  AA::Teacher teacher("Smith", "Jane", "T500", "jsmith@school.com", school);
  AA::Test test("History Quiz", teacher, 9);

  AA::TestVec questions = {
      AA::QandA("Who was the first president?", "Washington"),
      AA::QandA("What year was independence?", "1776"),
      AA::QandA("Which continent is Egypt in?", "Africa")};

  test.setTest(questions);
  EXPECT_EQ(test.getTest().size(), 3);
  EXPECT_EQ(test.getTest()[0].getQuestion(), "Who was the first president?");

  // Test random method returns a TestVec with shuffled/equal contents
  AA::TestVec randomized = test.random();
  EXPECT_EQ(randomized.size(), 3);

  std::cout << "[PASS] AA::Test question vector and random shuffling passed.\n";
}

TEST(TestClassTest, DueDateSortingAndEquality) {
  std::cout << "[TEST] Testing AA::Test due-date sorting (<=>) and equality "
               "operator (==)...\n";

  AA::School school("State High", "456 College Ave", "GradebookSYS");
  AA::Teacher teacher("Smith", "Jane", "T500", "jsmith@school.com", school);

  AA::Test test1("Biology Exam", teacher, 10);
  test1.setSubject("Science");

  AA::Test test2("Biology Exam", teacher, 10);
  test2.setSubject("Science");

  // Equality check
  EXPECT_TRUE(test1 == test2);

  std::cout
      << "[PASS] AA::Test due-date sorting and equality operator passed.\n";
}

TEST(TestClassTest, ClearFunction) {
  std::cout << "[TEST] Testing AA::Test clear() functionality...\n";

  AA::School school("State High", "456 College Ave", "GradebookSYS");
  AA::Teacher teacher("Smith", "Jane", "T500", "jsmith@school.com", school);

  AA::Test test("Temp Test", teacher, 8);
  EXPECT_EQ(test.getTestName(), "Temp Test");

  test.clear();
  EXPECT_EQ(test.getTestName(), "");
  EXPECT_EQ(test.getLevel(), 0);

  std::cout << "[PASS] AA::Test clear() functionality passed.\n";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
