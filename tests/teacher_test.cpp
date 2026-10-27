#include "Teacher.h"

#include <gtest/gtest.h>

#include "School.h"

// Test fixture or helper for common setup
class TeacherTest : public ::testing::Test {
 protected:
  AA::School defaultSchool{"Lincoln High", "123 Education St", "Gradebook_A"};
  AA::School otherSchool{"Washington High", "456 Learning Ave", "Gradebook_B"};
};

// 1. Testing Default Constructor
TEST_F(TeacherTest, DefaultConstructor) {
  AA::Teacher teacher;

  EXPECT_EQ(teacher.getLName(), "");
  EXPECT_EQ(teacher.getFName(), "");
  EXPECT_EQ(teacher.getTeacherID(), "");
  EXPECT_EQ(teacher.getEmailAddr(), "");
  EXPECT_EQ(teacher.getName(), ", ");
}

// 2. Testing Parameterized Constructor and Getters
TEST_F(TeacherTest, ParameterizedConstructorAndGetters) {
  AA::Teacher teacher("Doe", "John", "T10045", "john.doe@school.com",
                      defaultSchool);

  EXPECT_EQ(teacher.getLName(), "Doe");
  EXPECT_EQ(teacher.getFName(), "John");
  EXPECT_EQ(teacher.getTeacherID(), "T10045");
  EXPECT_EQ(teacher.getEmailAddr(), "john.doe@school.com");
  EXPECT_EQ(teacher.getName(), "Doe, John");
  EXPECT_EQ(teacher.getSchool().getName(), defaultSchool.getName());
}

// 3. Testing Setters
TEST_F(TeacherTest, Setters) {
  AA::Teacher teacher;

  teacher.setLName("Smith");
  teacher.setFName("Jane");
  teacher.setTeacherID("T99999");
  teacher.setEmailAddr("jane.smith@school.com");
  teacher.setSchool(otherSchool);

  EXPECT_EQ(teacher.getLName(), "Smith");
  EXPECT_EQ(teacher.getFName(), "Jane");
  EXPECT_EQ(teacher.getTeacherID(), "T99999");
  EXPECT_EQ(teacher.getEmailAddr(), "jane.smith@school.com");
  EXPECT_EQ(teacher.getName(), "Smith, Jane");
}

// 4. Testing Clear Function
TEST_F(TeacherTest, ClearFunction) {
  AA::Teacher teacher("Doe", "John", "T10045", "john.doe@school.com",
                      defaultSchool);

  // Verify it's populated first
  EXPECT_FALSE(teacher.getTeacherID().empty());

  // Clear it
  teacher.clear();

  // Verify it returned to default state
  EXPECT_EQ(teacher.getLName(), "");
  EXPECT_EQ(teacher.getFName(), "");
  EXPECT_EQ(teacher.getTeacherID(), "");
  EXPECT_EQ(teacher.getEmailAddr(), "");
}

// 5. Testing Equality Operator (operator==)
TEST_F(TeacherTest, EqualityOperator) {
  AA::Teacher t1("Doe", "John", "T10045", "john.doe@school.com", defaultSchool);
  AA::Teacher t2("Smith", "Jane", "T10045", "john.doe@school.com",
                 defaultSchool);  // Same ID, email, school
  AA::Teacher t3("Doe", "John", "T10046", "john.doe@school.com",
                 defaultSchool);  // Different ID
  AA::Teacher t4("Doe", "John", "T10045", "other.email@school.com",
                 defaultSchool);  // Different email

  EXPECT_TRUE(t1 == t2);
  EXPECT_FALSE(t1 == t3);
  EXPECT_FALSE(t1 == t4);
}

// 6. Testing Three-Way Comparison Operator (operator<=>)
TEST_F(TeacherTest, SpaceshipOperator) {
  AA::School schoolA("A School", "1 St", "GB1");
  AA::School schoolB("B School", "2 St", "GB2");

  AA::Teacher t1("Alice", "A", "1", "a@school.com", schoolA);
  AA::Teacher t2("Bob", "B", "2", "b@school.com", schoolA);
  AA::Teacher t3("Charlie", "C", "3", "a@school.com", schoolB);

  // t1 < t2 (same school, email comparison)
  EXPECT_TRUE((t1 <=> t2) < 0);

  // t1 < t3 (schoolA < schoolB)
  EXPECT_TRUE((t1 <=> t3) < 0);
}

// 7. Testing nlohmann/json Serialization and Deserialization
TEST_F(TeacherTest, JsonSerialization) {
  AA::Teacher original("Doe", "John", "T10045", "john.doe@school.com",
                       defaultSchool);

  // Convert to JSON
  nlohmann::json j = original;

  // Convert back from JSON
  AA::Teacher restored = j.get<AA::Teacher>();

  // Verify fields match
  EXPECT_EQ(original.getLName(), restored.getLName());
  EXPECT_EQ(original.getFName(), restored.getFName());
  EXPECT_EQ(original.getTeacherID(), restored.getTeacherID());
  EXPECT_EQ(original.getEmailAddr(), restored.getEmailAddr());
  EXPECT_TRUE(original == restored);
}
