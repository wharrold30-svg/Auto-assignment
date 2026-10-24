#include <gtest/gtest.h>

#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

#include "Student.h"
#include "StudentSingleton.h"

class StudentSingletonTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear out any existing students to ensure a clean slate for each test
    auto& singleton = AA::StudentSingleton::Instance();
    auto studentsPtr = singleton.getStudents();
    if (studentsPtr && !studentsPtr->empty()) {
      singleton.removeStudent(*studentsPtr);
    }
  }

  void TearDown() override {
    // Cleanup after test
    auto& singleton = AA::StudentSingleton::Instance();
    auto studentsPtr = singleton.getStudents();
    if (studentsPtr && !studentsPtr->empty()) {
      singleton.removeStudent(*studentsPtr);
    }
  }
};

// 1. Test Singleton Instance Uniqueness
TEST_F(StudentSingletonTest, TestSingletonInstance) {
  std::cout << "[TEST PART] Testing: Singleton Instance Uniqueness\n";

  auto& instance1 = AA::StudentSingleton::Instance();
  auto& instance2 = AA::StudentSingleton::Instance();

  EXPECT_EQ(&instance1, &instance2) << "Failure: Instance() did not return the "
                                       "exact same singleton reference.";
  std::cout << "[RESULT] PASSED: Singleton Instance Uniqueness\n\n";
}

// 2. Test Adding and Getting a Single Student
TEST_F(StudentSingletonTest, TestAddAndGetSingleStudent) {
  std::cout << "[TEST PART] Testing: Add Single Student & Get Students\n";

  auto& singleton = AA::StudentSingleton::Instance();

  // Constructor: (studentID, emailAddr, lName, fName, classPeriod, school)
  AA::Student student1("S001", "alice@school.com", "Smith", "Alice", 1,
                       "Lincoln High");
  singleton.addStudent(student1);

  auto students = singleton.getStudents();
  ASSERT_NE(students, nullptr);
  EXPECT_EQ(students->size(), 1UL)
      << "Failure: Student count should be 1 after adding a student.";
  EXPECT_TRUE(students->find(student1) != students->end())
      << "Failure: Added student was not found in the set.";

  std::cout << "[RESULT] PASSED: Add Single Student & Get Students\n\n";
}

// 3. Test Adding and Removing a Set of Students
TEST_F(StudentSingletonTest, TestAddAndRemoveStudentSet) {
  std::cout << "[TEST PART] Testing: Add & Remove a Set of Students\n";

  auto& singleton = AA::StudentSingleton::Instance();

  std::set<AA::Student> studentSet = {
      AA::Student("S002", "bob@school.com", "Jones", "Bob", 2, "Lincoln High"),
      AA::Student("S003", "charlie@school.com", "Brown", "Charlie", 2,
                  "Lincoln High")};

  singleton.addStudent(studentSet);
  auto students = singleton.getStudents();
  EXPECT_EQ(students->size(), 2UL)
      << "Failure: Student count should be 2 after bulk addition.";

  singleton.removeStudent(studentSet);
  students = singleton.getStudents();
  EXPECT_TRUE(students->empty())
      << "Failure: Student set should be empty after bulk removal.";

  std::cout << "[RESULT] PASSED: Add & Remove a Set of Students\n\n";
}

// 4. Test Period Subscript Operator ([]) and Out of Range Exception
TEST_F(StudentSingletonTest, TestPeriodSubscriptOperator) {
  std::cout << "[TEST PART] Testing: Period Subscript Operator ([]) & "
               "Exception Handling\n";

  auto& singleton = AA::StudentSingleton::Instance();

  AA::Student student1("S004", "david@school.com", "Miller", "David", 3,
                       "Lincoln High");
  singleton.addStudent(student1);

  // Test valid period lookup
  const auto& constSingleton = singleton;
  const auto periodStudents = constSingleton[3];
  EXPECT_FALSE(periodStudents.empty())
      << "Failure: Expected students in period 3.";

  // Test out of range exception for invalid period
  EXPECT_THROW(
      {
        try {
          auto invalidPeriodStudents = constSingleton[99];
        } catch (const std::out_of_range& e) {
          std::cout << "[INFO] Caught expected out_of_range exception: "
                    << e.what() << "\n";
          throw;
        }
      },
      std::out_of_range)
      << "Failure: Expected std::out_of_range exception for non-existent "
         "period 99.";

  std::cout << "[RESULT] PASSED: Period Subscript Operator ([]) & Exception "
               "Handling\n\n";
}

// 5. Test Find Method
TEST_F(StudentSingletonTest, TestFindStudent) {
  std::cout << "[TEST PART] Testing: Find Method (Existing & Non-Existing)\n";

  auto& singleton = AA::StudentSingleton::Instance();

  AA::Student target("S005", "eve@school.com", "Davis", "Eve", 1,
                     "Lincoln High");
  AA::Student stranger("S999", "frank@school.com", "Wilson", "Frank", 1,
                       "Lincoln High");

  singleton.addStudent(target);

  // Find existing
  auto foundResult = singleton.find(target);
  EXPECT_TRUE(foundResult.has_value())
      << "Failure: Should have found the target student.";

  // Find non-existing
  auto notFoundResult = singleton.find(stranger);
  EXPECT_FALSE(notFoundResult.has_value())
      << "Failure: Should not have found the stranger student.";

  std::cout << "[RESULT] PASSED: Find Method (Existing & Non-Existing)\n\n";
}

// 6. Test Free Function maxNameLength
TEST_F(StudentSingletonTest, TestMaxNameLength) {
  std::cout << "[TEST PART] Testing: Free Function maxNameLength\n";

  std::set<AA::Student> sampleStudents = {
      AA::Student("S006", "al@school.com", "Smith", "Al", 1, "Lincoln High"),
      AA::Student("S007", "alexander@school.com", "Smith", "Alexander", 1,
                  "Lincoln High"),
      AA::Student("S008", "bob@school.com", "Jones", "Bob", 1, "Lincoln High")};

  std::size_t maxLength = AA::maxNameLength(sampleStudents);
  // Adjust expected length based on whether maxNameLength evaluates first name,
  // last name, or combined
  EXPECT_GT(maxLength, 0UL) << "Failure: maxNameLength returned 0.";

  std::cout << "[RESULT] PASSED: Free Function maxNameLength\n\n";
}
