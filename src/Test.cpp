#include "Test.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <random>

#include "Log.h"
#include "Student.h"
#include "nlohmann/json.hpp"

std::optional<AA::Test::studentGradeMap> AA::Test::getStudents(
    const std::size_t period) const {
  if (m_students.empty()) {
    return std::nullopt;
  }
  studentGradeMap sgm;

  for (const auto& it : m_students) {
    if (period == 0 || period == it.getClassPeriod()) {
      sgm.insert({std::make_shared<Student>(static_cast<const Student&>(it)),
                  it.getGrade()});
    }
  }
  return sgm;
}

std::vector<std::string> AA::Test::getStudentTest(
    const std::shared_ptr<Student>& student) const {
  studentExt temp{student};
  const auto loc{m_students.find(temp)};
  if (loc != m_students.end()) {
    return loc->getReturnTest();
  }
  return {};
}

bool AA::Test::setStudentTest(const std::shared_ptr<Student>& student,
                              const std::vector<std::string>& test,
                              const DT::Date& rcvDate, std::size_t id) {
  studentExt temp{student, id};

  auto loc{m_students.find(temp)};
  if (loc == m_students.end()) {
    return false;
  }
  auto node = m_students.extract(loc);

  node.value().setReturnedTest(test);
  node.value().setRcvDate(rcvDate);

  m_students.insert(std::move(node));
  return true;
}

void AA::Test::gradeTests() {
  if (m_test.empty()) {
    return;
  }

  // Helper lambda that grades a mutable studentExt object in one pass
  auto gradeOne = [this](studentExt& s) {
    const auto& answers = s.getReturnTest();
    if (answers.empty()) {
      s.setGrade(0.0f);
      return;
    }

    std::size_t wrongCount = 0;
    for (std::size_t i{0}; i < m_test.size(); ++i) {
      const auto& [question, answer] = m_test[i];
      if (i >= answers.size() || answer != answers[i]) {
        s.setWrongAnswer(i);
        ++wrongCount;
      }
    }

    const std::size_t correct = m_test.size() - wrongCount;
    s.setGrade((static_cast<float>(correct) / m_test.size()) * 100.0f);
  };

  // Safe iterator-advancing extraction loop (C++17)
  for (auto it = m_students.begin(); it != m_students.end();) {
    // Advance iterator BEFORE extracting the current node
    auto node = m_students.extract(it++);
    gradeOne(node.value());
    m_students.insert(std::move(node));
  }
}

void AA::Test::addStudent(const std::shared_ptr<Student>& student,
                          const std::size_t id) {
  studentExt temp{student, id};
  m_students.insert(std::move(temp));
}

void AA::Test::removeStudent(const std::shared_ptr<Student>& student) {
  studentExt temp{student};
  m_students.erase(temp);
}

bool AA::Test::hasStudent(const std::shared_ptr<Student>& student) const {
  studentExt temp{student};
  return m_students.contains(temp);
}

bool AA::Test::rcvTest(const std::shared_ptr<Student>& student) const {
  studentExt temp{student};
  auto loc{m_students.find(temp)};
  if (loc != m_students.end()) {
    return !loc->getReturnTest().empty();
  }
  return false;
}

AA::TestVec AA::Test::random() const {
  TestVec ret{m_test};

  std::random_device rd;

  std::mt19937 mt(rd());

  std::shuffle(ret.begin(), ret.end(), mt);

  return ret;
}
AA::Test::operator AA::Test::studentGradeMap() const {
  return getStudents().value_or(studentGradeMap{});
}

namespace fs = std::filesystem;
void AA::saveTest(const AA::Test& test) {
  fs::path target_dir = "Tests";
  fs::path file_name = test.getTestName() + ".json";

  fs::path full_path = target_dir / file_name;

  try {
    // Ensure the complete directory tree exists
    // This will safely do nothing if the folders already exist
    fs::create_directories(target_dir);

    // Open and write to the file using the full path
    std::ofstream outfile(full_path);

    if (outfile.is_open()) {
      nlohmann::json j{test};
      outfile << j;
      outfile.close();
    } else {
    }
  } catch (const fs::filesystem_error& e) {
    AA::SaveErrorLog(e.what());
  }
}

AA::Test AA::loadTest(std::string_view testName) {
  fs::path target_dir = "Tests";
  fs::path file_name = std::string{testName} + ".json";
  fs::path full_path = target_dir / file_name;

  Test ret;
  // Safety Check: Verify that the file actually exists down the line
  if (!fs::exists(full_path)) {
    LoadErrorLog("Could not load" + std::string{testName});
    throw std::runtime_error{"Could not load" + std::string{testName}};
  }

  try {
    // Open the file for reading (ifstream)
    std::ifstream infile(full_path);

    if (infile.is_open()) {
      nlohmann::json j;

      // Stream the file directly into the json object
      infile >> j;
      infile.close();

      // Access the data exactly like you would an object or dictionary
      ret = j;
    } else {
      LoadErrorLog("Could not load" + std::string{testName});
      throw std::runtime_error{"Could not load" + std::string{testName}};
    }
  } catch (const nlohmann::json::parse_error& e) {
    LoadErrorLog("Could not load" + std::string{testName});
    throw std::runtime_error{"Could not load" + std::string{testName}};
  } catch (const fs::filesystem_error& e) {
    LoadErrorLog("Could not load" + std::string{testName});
    throw std::runtime_error{"Could not load" + std::string{testName}};
  }

  return ret;
}

std::size_t AA::maxNameLength(const AA::Test::studentGradeMap& students) {
  std::size_t max{0};

  for (const auto& [student, grade] : students) {
    max = std::max(max, student->getName().size());
  }
  return max;
}

std::ostream& operator<<(std::ostream& os, const AA::Test& test) {
  os << std::format("Test name: {}\tSubject:{}\tLevel: {}\n",
                    test.getTestName(), test.getSubject(), test.getLevel());

  os << "Description: " << test.getDescription();

  os << "\n\nQuestion and Answers:\n";
  std::size_t n{1};
  for (const auto& t = test.getTest(); const auto& [question, answer] : t) {
    os << std::format("{:3}) {}\n{}\n\n", n++, question, answer);
  }

  const auto foundStudents{test.getStudents()};
  if (!foundStudents) {
    os << test.getTestName() << " has no students\n";
    return os;
  }

  os << "Given on: " << test.getSentDate() << "\n";
  os << "Due date: " << test.getDueDate() << "\n";

  const auto& students{foundStudents.value()};
  const std::size_t fieldWidth{AA::maxNameLength(students)};
  for (const auto& [student, grade] : students) {
    os << std::format("{:{}} | {:3}\n", student->getName(), fieldWidth, grade);
    // separator
    os << std::string(fieldWidth * 2 + 6, '-');
  }
  // give a report once ive created the class
  // AA::Report report{test};
  // os << "\n" << report;
  return os;
}
std::optional<DT::Date> AA::Test::getRcvDate(
    const std::shared_ptr<Student>& student) const {
  if (!rcvTest(student)) {
    return std::nullopt;
  }
  studentExt temp{student};
  auto pos{m_students.find(temp)};
  return pos->getRcvDate();
}
float AA::Test::getGrade(const std::shared_ptr<Student>& student) const {
  if (!rcvTest(student)) {
    return 0.0f;
  }
  studentExt temp{student};
  auto pos{m_students.find(temp)};
  return pos->getGrade();
}

std::optional<std::vector<std::size_t>> AA::Test::getWrongAnswers(
    const std::shared_ptr<Student>& student) const {
  if (!rcvTest(student)) {
    return std::nullopt;
  }
  studentExt temp{student};
  auto pos{m_students.find(temp)};
  return pos->getWrongAnswers();
}
std::string AA::Test::generateID() {
  std::string ret;
  std::size_t idLen{15};
  ret.reserve(idLen);

  // Resize the string down to the actual number of characters written
  static const char alphanum[] =
      "0123456789"
      "abcdefghijklmnopqrstuvexyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const int strLen{std::size(alphanum) - 1};

  std::random_device rd;
  std::mt19937 mt(rd());
  while (ret.size() < idLen) {
    std::uniform_int_distribution<int> dist(0, strLen - 1);
    ret += alphanum[dist(mt)];
  }
  return ret;
}
