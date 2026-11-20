#ifndef TESTS_FAKES_FAKE_STORE_H
#define TESTS_FAKES_FAKE_STORE_H

#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "GradeRecord.h"
#include "Gradebook.h"
#include "OpenTest.h"
#include "School.h"
#include "Student.h"
#include "Teacher.h"
#include "Test.h"

namespace AA {
namespace Fake {

struct Credentials {
  std::string salt;
  std::string password;
};

struct RosterEntry {
  std::string teacherId;
  std::string studentId;
  unsigned int classPeriod = 0;
};

struct ReturnedTestEntry {
  int openTestId = 0;  // Matches OpenTest::getID() / open_tests.id
  std::string testId;  // Matches AA::Test::getID() / tests.test_id
  std::string
      wrongAnswers;  // Stored on the returned submission record, not OpenTest
  AA::Test returnedTest;
};

struct Store {
  // school_name -> School
  std::map<std::string, AA::School> schools;
  // gradebook_name -> Gradebook
  std::map<std::string, AA::Gradebook> gradebooks;
  // teacher_id -> Teacher
  std::unordered_map<std::string, AA::Teacher> teachers;
  // teacher_id -> Credentials
  std::unordered_map<std::string, Credentials> credentials;
  // student_id -> Student
  std::unordered_map<std::string, AA::Student> students;
  // test_id -> Test
  std::unordered_map<std::string, AA::Test> tests;
  // test_id -> questions
  std::unordered_map<std::string, std::vector<AA::QandA>> testQuestions;
  // teacher_id -> OpenTest list
  std::unordered_map<std::string, std::vector<AA::OpenTest>> openTestsByTeacher;
  // student_id -> OpenTest list
  std::unordered_map<std::string, std::vector<AA::OpenTest>> openTestsByStudent;
  // student_id -> GradeRecord list
  std::unordered_map<std::string, std::vector<AA::GradeRecord>>
      assignmentsByStudent;
  // student_id -> returned AA::Test submissions
  std::unordered_map<std::string, std::vector<ReturnedTestEntry>>
      returnedTestsByStudent;
  // Roster entries
  std::vector<RosterEntry> roster;
};

}  // namespace Fake
}  // namespace AA

#endif
