#include "Test.h"

#include <algorithm>
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
                              const std::vector<std::string>& test) {
  studentExt temp{student};

  auto loc{m_students.find(temp)};
  if (loc == m_students.end()) {
    return false;
  }
  auto node = m_students.extract(loc);

  node.value().setReturnedTest(test);

  m_students.insert(std::move(node));
  return true;
}

void AA::Test::gradeTests() {
  auto setGrade = [this](const studentExt& temp, const float grade) -> bool {
    auto loc{m_students.find(temp)};
    if (loc == m_students.end()) {
      return false;
    }
    auto node = m_students.extract(loc);

    node.value().setGrade(grade);

    m_students.insert(std::move(node));
    return true;
  };
  auto setWrongAnswer = [this](const studentExt& temp,
                               const std::size_t num) -> bool {
    auto loc{m_students.find(temp)};
    if (loc == m_students.end()) {
      return false;
    }
    auto node = m_students.extract(loc);

    node.value().setWrongAnswer(num);

    m_students.insert(std::move(node));
    return true;
  };

  for (const auto& student : m_students) {
    // if a test hasnt been returned then set it to 0.0
    if (student.getReturnTest().empty()) {
      setGrade(student, 0.0f);
      continue;
    }

    // Compare the student's answer to the correct answer
    // insert the number of the incorrect answer into the
    // student's wrong answets
    const auto& studentAnswers = student.getReturnTest();

    for (std::size_t i{0}; i < m_test.size(); ++i) {
      const auto& [question, answer] = m_test[i];

      if (answer != studentAnswers.at(i)) {
        setWrongAnswer(student, i);
      }
    }
    // calculate the grade
    const std::size_t correctAnswers{m_test.size() -
                                     student.getWrongAnswerd().size()};
    const float grade{static_cast<float>(correctAnswers) / m_test.size()};

    setGrade(student, grade * 100);
  }
}
void AA::Test::addStudent(const std::shared_ptr<Student>& student) {
  studentExt temp{student};
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
    os << test.getTestName() << " has no students/n";
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
