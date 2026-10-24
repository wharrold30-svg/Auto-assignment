#include "Student.h"

#include <gtest/gtest.h>

#include <sstream>

// ==========================================
// 1. Testing Default Constructor & Initial State
// ==========================================
TEST(StudentTest, DefaultConstructor) {
  std::cout << "[TESTING] Default Constructor and Initial State...\n";
  AA::Student student;

  EXPECT_EQ(student.getStudentID(), "");
  EXPECT_EQ(student.getEmailAddr(), "");
  EXPECT_EQ(student.getLName(), "");
  EXPECT_EQ(student.getFName(), "");
  EXPECT_EQ(student.getSchool(), "");
  EXPECT_TRUE(student.getTests().empty());
  std::cout << "[PASSED] Default Constructor initialized with default/empty "
               "values.\n";
}

// ==========================================
// 2. Testing Parameterized Constructor & Getters
// ==========================================
TEST(StudentTest, ParameterizedConstructorAndGetters) {
  std::cout << "[TESTING] Parameterized Constructor and Getters...\n";
  AA::Student student("S12345", "john.doe@school.com", "Doe", "John", 2,
                      "Lincoln High");

  EXPECT_EQ(student.getStudentID(), "S12345");
  EXPECT_EQ(student.getEmailAddr(), "john.doe@school.com");
  EXPECT_EQ(student.getLName(), "Doe");
  EXPECT_EQ(student.getFName(), "John");
  EXPECT_EQ(student.getClassPeriod(), 2);
  EXPECT_EQ(student.getSchool(), "Lincoln High");
  std::cout << "[PASSED] Parameterized Constructor and Getters return correct "
               "values.\n";
}

// ==========================================
// 3. Testing Setters
// ==========================================
TEST(StudentTest, Setters) {
  std::cout << "[TESTING] Student Setters...\n";
  AA::Student student;

  student.setStudentID("S99999");
  student.setEmailAddr("jane.smith@school.com");
  student.setLName("Smith");
  student.setFName("Jane");
  student.setClassPeriod(4);
  student.setSchool("Washington High");

  EXPECT_EQ(student.getStudentID(), "S99999");
  EXPECT_EQ(student.getEmailAddr(), "jane.smith@school.com");
  EXPECT_EQ(student.getLName(), "Smith");
  EXPECT_EQ(student.getFName(), "Jane");
  EXPECT_EQ(student.getClassPeriod(), 4);
  EXPECT_EQ(student.getSchool(), "Washington High");
  std::cout << "[PASSED] All setters successfully updated internal member "
               "variables.\n";
}

// ==========================================
// 4. Testing Name Formatting (`getName`)
// ==========================================
TEST(StudentTest, NameFormatting) {
  std::cout << "[TESTING] getName() format (Last First)...\n";
  AA::Student student("", "", "Wayne", "Bruce", 1, "");

  EXPECT_EQ(student.getName(), "Wayne Bruce");
  std::cout
      << "[PASSED] getName() correctly formats name as 'LastName FirstName'.\n";
}

// ==========================================
// 5. Testing Test Management (Add, Remove, Clear)
// ==========================================
TEST(StudentTest, TestManagementOperations) {
  std::cout << "[TESTING] Test Management (addTest, removeTest, clear)...\n";
  AA::Student student("S1", "test@test.com", "Test", "User", 1, "School");

  DT::Date d1("2026-06-01 10:00:00");
  DT::Date d2("2026-06-02 11:30:00");

  student.addTest("Midterm", 95.5f, d1);
  student.addTest("Final", 88.0f, d2);

  ASSERT_EQ(student.getTests().size(), 2);
  EXPECT_EQ(student.getTests()[0].testName, "Midterm");
  EXPECT_EQ(student.getTests()[0].grade, 95.5f);

  student.removeTest("Midterm", d1);
  EXPECT_EQ(student.getTests().size(), 1);
  EXPECT_EQ(student.getTests()[0].testName, "Final");

  student.clear();
  EXPECT_EQ(student.getStudentID(), "");
  EXPECT_TRUE(student.getTests().empty());
  std::cout << "[PASSED] Test management functions (add, remove, clear) work "
               "as expected.\n";
}

// ==========================================
// 6. Testing Comparison Operators (`==` and `<=>`)
// ==========================================
TEST(StudentTest, ComparisonOperators) {
  std::cout
      << "[TESTING] Comparison operators (`operator==` and `operator<=>`)...\n";

  AA::Student s1("S100", "a@a.com", "Adams", "Alice", 1, "High");
  AA::Student s2("S200", "b@b.com", "Baker", "Bob", 1,
                 "High");  // Different ID -> evaluates to false for equality
  AA::Student s3("S200", "c@c.com", "Adams", "Alice", 2, "High");

  // Equality: matches if class period and student ID are equal
  AA::Student s1Duplicate("S100", "different@a.com", "Adams", "Alice", 1,
                          "Different");
  EXPECT_TRUE(s1 == s1Duplicate);
  EXPECT_FALSE(s1 == s2);  // Now correctly compares different IDs

  // Spaceship operator: sorted by class period first, then name
  EXPECT_TRUE(s1 < s3);  // Period 1 comes before Period 2

  AA::Student sA("S101", "a@a.com", "Adams", "Charlie", 1, "High");
  AA::Student sB("S102", "b@b.com", "Zack", "Alice", 1, "High");
  EXPECT_TRUE(sA < sB);  // Same period, Adams Charlie comes before Zack Alice

  std::cout << "[PASSED] Comparison operators evaluated correctly.\n";
}

// ==========================================
// 7. Testing Stream Insertion Operator (`operator<<`)
// ==========================================
TEST(StudentTest, StreamInsertionOperator) {
  std::cout << "[TESTING] Stream Insertion Operator (`operator<<`)...\n";
  AA::Student student("S123", "student@school.com", "Smith", "John", 3,
                      "East High");

  std::ostringstream oss;
  oss << student;

  std::string expectedOutput =
      "S123\nstudent@school.com\nSmith John\n3\nEast High\n";
  EXPECT_EQ(oss.str(), expectedOutput);
  std::cout << "[PASSED] Stream insertion operator outputs the correct "
               "formatted string.\n";
}

// ==========================================
// 8. Testing JSON Serialization (`nlohmann/json`)
// ==========================================
TEST(StudentTest, JsonSerialization) {
  std::cout << "[TESTING] nlohmann/json Intrusive Serialization...\n";
  AA::Student original("S789", "json@test.com", "Json", "Tester", 5,
                       "Tech High");

  nlohmann::json j = original;
  AA::Student restored = j.get<AA::Student>();

  EXPECT_EQ(restored.getStudentID(), original.getStudentID());
  EXPECT_EQ(restored.getEmailAddr(), original.getEmailAddr());
  EXPECT_EQ(restored.getLName(), original.getLName());
  EXPECT_EQ(restored.getFName(), original.getFName());
  EXPECT_EQ(restored.getClassPeriod(), original.getClassPeriod());
  EXPECT_EQ(restored.getSchool(), original.getSchool());
  std::cout << "[PASSED] JSON serialization and deserialization preserved "
               "student fields successfully.\n";
}
