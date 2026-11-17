#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Student.h"
#include "Teacher.h"
#include "Test.h"
#include "sql/IDataRepository.h"

// ==========================================
// 1. Mark Test Double implementing every method in IDataRepository
// ==========================================
class Mark : public AA::IDataRepository {
 public:
  // Tracking and stub behavior flags
  bool shouldFail = false;

  AA::Student cannedStudent;
  AA::Teacher cannedTeacher;
  AA::Test cannedTest;
  AA::School cannedSchool;

  int saveStudentCallCount = 0;
  int loadStudentCallCount = 0;
  int saveTeacherCallCount = 0;
  int loadTeacherCallCount = 0;
  int saveTestCallCount = 0;
  int loadTestCallCount = 0;
  int saveSchoolCallCount = 0;
  int loadSchoolCallCount = 0;
  int enrollStudentCallCount = 0;
  int saveTeacherRosterCallCount = 0;

  std::string lastStudentId;
  std::string lastTeacherId;
  std::string lastTestId;
  std::string lastSchoolName;

  bool saveStudent(const AA::Student& student) override {
    saveStudentCallCount++;
    return !shouldFail;
  }

  bool loadStudent(const std::string& student_id,
                   AA::Student& out_student) override {
    loadStudentCallCount++;
    lastStudentId = student_id;
    if (shouldFail) return false;
    out_student = cannedStudent;
    return true;
  }

  bool saveTeacher(const AA::Teacher& teacher) override {
    saveTeacherCallCount++;
    return !shouldFail;
  }

  bool loadTeacher(const std::string& teacher_id,
                   AA::Teacher& out_teacher) override {
    loadTeacherCallCount++;
    lastTeacherId = teacher_id;
    if (shouldFail) return false;
    out_teacher = cannedTeacher;
    return true;
  }

  bool saveTest(const AA::Test& test) override {
    saveTestCallCount++;
    return !shouldFail;
  }

  bool loadTest(const std::string& test_id, AA::Test& out_test) override {
    loadTestCallCount++;
    lastTestId = test_id;
    if (shouldFail) return false;
    out_test = cannedTest;
    return true;
  }

  bool saveSchool(const AA::School& school) override {
    saveSchoolCallCount++;
    return !shouldFail;
  }

  bool loadSchool(const std::string& school_name,
                  AA::School& out_school) override {
    loadSchoolCallCount++;
    lastSchoolName = school_name;
    if (shouldFail) return false;
    out_school = cannedSchool;
    return true;
  }

  bool enrollStudentInClass(const std::string& teacherId,
                            const std::string& studentId,
                            unsigned int classPeriod) override {
    enrollStudentCallCount++;
    return !shouldFail;
  }

  bool saveTeacherRoster(
      const std::string& teacherId,
      const std::vector<std::pair<std::string, unsigned int>>&
          studentPeriodPairs) override {
    saveTeacherRosterCallCount++;
    return !shouldFail;
  }
};

// ==========================================
// 2. Google Unit Tests for IDataRepository Interface via Mark
// ==========================================
TEST(DataRepositoryInterfaceTest, StudentSaveAndLoad) {
  auto mark = std::make_shared<Mark>();

  AA::Student dummyStudent;
  EXPECT_TRUE(mark->saveStudent(dummyStudent));
  EXPECT_EQ(mark->saveStudentCallCount, 1);

  AA::Student loadedStudent;
  EXPECT_TRUE(mark->loadStudent("S123", loadedStudent));
  EXPECT_EQ(mark->loadStudentCallCount, 1);
  EXPECT_EQ(mark->lastStudentId, "S123");
}

TEST(DataRepositoryInterfaceTest, TeacherSaveAndLoad) {
  auto mark = std::make_shared<Mark>();

  AA::Teacher dummyTeacher;
  EXPECT_TRUE(mark->saveTeacher(dummyTeacher));
  EXPECT_EQ(mark->saveTeacherCallCount, 1);

  AA::Teacher loadedTeacher;
  EXPECT_TRUE(mark->loadTeacher("T456", loadedTeacher));
  EXPECT_EQ(mark->loadTeacherCallCount, 1);
  EXPECT_EQ(mark->lastTeacherId, "T456");
}

TEST(DataRepositoryInterfaceTest, TestSaveAndLoad) {
  auto mark = std::make_shared<Mark>();

  AA::Test dummyTest;
  EXPECT_TRUE(mark->saveTest(dummyTest));
  EXPECT_EQ(mark->saveTestCallCount, 1);

  AA::Test loadedTest;
  EXPECT_TRUE(mark->loadTest("TEST_001", loadedTest));
  EXPECT_EQ(mark->loadTestCallCount, 1);
  EXPECT_EQ(mark->lastTestId, "TEST_001");
}

TEST(DataRepositoryInterfaceTest, SchoolSaveAndLoad) {
  auto mark = std::make_shared<Mark>();

  AA::School dummySchool;
  EXPECT_TRUE(mark->saveSchool(dummySchool));
  EXPECT_EQ(mark->saveSchoolCallCount, 1);

  AA::School loadedSchool;
  EXPECT_TRUE(mark->loadSchool("Lincoln High", loadedSchool));
  EXPECT_EQ(mark->loadSchoolCallCount, 1);
  EXPECT_EQ(mark->lastSchoolName, "Lincoln High");
}

TEST(DataRepositoryInterfaceTest, ClassEnrollmentAndRoster) {
  auto mark = std::make_shared<Mark>();

  // Test enrollStudentInClass
  EXPECT_TRUE(mark->enrollStudentInClass("T456", "S123", 2));
  EXPECT_EQ(mark->enrollStudentCallCount, 1);

  // Test saveTeacherRoster
  std::vector<std::pair<std::string, unsigned int>> roster = {{"S123", 1},
                                                              {"S124", 2}};
  EXPECT_TRUE(mark->saveTeacherRoster("T456", roster));
  EXPECT_EQ(mark->saveTeacherRosterCallCount, 1);
}

TEST(DataRepositoryInterfaceTest, FailureHandling) {
  auto mark = std::make_shared<Mark>();
  mark->shouldFail = true;

  AA::Student student;
  AA::Teacher teacher;
  AA::Test test;
  AA::School school;

  EXPECT_FALSE(mark->saveStudent(student));
  EXPECT_FALSE(mark->loadStudent("S123", student));
  EXPECT_FALSE(mark->saveTeacher(teacher));
  EXPECT_FALSE(mark->loadTeacher("T456", teacher));
  EXPECT_FALSE(mark->saveTest(test));
  EXPECT_FALSE(mark->loadTest("TEST_001", test));
  EXPECT_FALSE(mark->saveSchool(school));
  EXPECT_FALSE(mark->loadSchool("SchoolName", school));
  EXPECT_FALSE(mark->enrollStudentInClass("T456", "S123", 1));
  EXPECT_FALSE(mark->saveTeacherRoster("T456", {}));
}

// ==========================================
// 3. Custom Event Listener to print test status
// ==========================================
class VerboseTestLogger : public ::testing::EmptyTestEventListener {
  void OnTestStart(const ::testing::TestInfo& test_info) override {
    std::cout << "[RUNNING] Test: " << test_info.test_case_name() << "."
              << test_info.name() << std::endl;
  }

  void OnTestEnd(const ::testing::TestInfo& test_info) override {
    if (test_info.result()->Passed()) {
      std::cout << "[PASSED]  Test: " << test_info.test_case_name() << "."
                << test_info.name() << "\n"
                << std::endl;
    } else {
      std::cout << "[FAILED]  Test: " << test_info.test_case_name() << "."
                << test_info.name() << "\n"
                << std::endl;
    }
  }
};

// ==========================================
// 4. Main function with summary check
// ==========================================
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // Add our custom logger to Google Test's listeners
  ::testing::TestEventListeners& listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new VerboseTestLogger);

  int result = RUN_ALL_TESTS();

  if (result == 0) {
    std::cout << "passed all tests" << std::endl;
  }
  return result;
}
