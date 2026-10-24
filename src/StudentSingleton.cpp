#include "StudentSingleton.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Student.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

std::optional<AA::Student> loadStudent(const fs::path& p) {
  std::ifstream in{p};
  if (!in) {
    return std::nullopt;
  }
  nlohmann::json j;
  in >> j;
  return j.get<AA::Student>();
}

void AA::StudentSingleton::load() {
  fs::path p = fs::current_path() / "Students";

  // Guard against missing directory on first run
  if (!fs::exists(p) || !fs::is_directory(p)) {
    return;
  }

  for (const auto& iter : fs::recursive_directory_iterator(p)) {
    auto foundStudent{loadStudent(iter.path())};
    if (foundStudent != std::nullopt) {
      m_students.insert(foundStudent.value());
    }
  }
}

void saveStudent(const fs::path& p, const AA::Student& s) {
  std::ofstream out{p};
  if (!out) {
    throw std::runtime_error{"could not save: " + s.getName()};
  }

  nlohmann::json j;
  j = s;

  out << j;
}

void AA::StudentSingleton::save() {
  fs::path root = fs::current_path() / "Students";

  // automatically creat the directory if it doesn't exit
  fs::create_directories(root);

  for (const auto& iter : m_students) {
    fs::path p = root / (iter.getName() + ".json");
    saveStudent(p, iter);
  }
}

AA::StudentSingleton::StudentSingleton() { load(); }

AA::StudentSingleton::~StudentSingleton() { save(); }

AA::StudentSingleton& AA::StudentSingleton::Instance() {
  static StudentSingleton inst;
  return inst;
}

void AA::StudentSingleton::addStudent(const Student& student) {
  m_students.insert(student);
}

void AA::StudentSingleton::addStudent(const std::set<Student>& students) {
  m_students.insert(students.begin(), students.end());
}

void AA::StudentSingleton::removeStudent(const Student& student) {
  m_students.erase(student);
}
void AA::StudentSingleton::removeStudent(const std::set<Student>& students) {
  for (const auto& s : students) {
    removeStudent(s);
  }
}

// 1. CONST VERSION (The "core" logic lives here)
const std::vector<const AA::Student*> AA::StudentSingleton::operator[](
    const std::size_t period) const {
  std::vector<const Student*> pStudentVec;

  for (const auto& s : m_students) {
    if (s.getClassPeriod() == period) {
      pStudentVec.push_back(&s);
    }
  }

  if (pStudentVec.empty()) {
    throw std::out_of_range{"No Students in this period: " +
                            std::to_string(period)};
  }
  return pStudentVec;
}

// 2. NON-CONST VERSION (Calls the const version and strips the constness)
std::vector<AA::Student*> AA::StudentSingleton::operator[](
    const std::size_t period) {
  // Call the const version first using static_cast
  auto constResults = static_cast<const AA::StudentSingleton&>(*this)[period];

  std::vector<Student*> nonConstResults;
  nonConstResults.reserve(constResults.size());

  // Use const_cast to strip the 'const' from the pointers safely
  for (const auto* s : constResults) {
    nonConstResults.push_back(const_cast<Student*>(s));
  }

  return nonConstResults;
}

std::size_t AA::maxNameLength(const std::set<Student>& students) {
  if (students.empty()) {
    return 0;
  }
  std::size_t max{0};
  std::string name;
  for (const auto& student : students) {
    name = student.getName();
    max = std::max(max, name.size());
  }
  return max;
}

std::ostream& operator<<(std::ostream& os,
                         const std::set<AA::Student>& students) {
  const std::size_t fieldWidth{AA::maxNameLength(students) + 2};
  std::size_t rowLength = 0;
  for (const auto& student : students) {
    auto tests{student.getTests()};
    if (tests.empty()) {
      continue;
    }
    rowLength = std::max(rowLength, tests.size());
  }

  for (const auto& student : students) {
    os << std::format("{:{}} |", student.getName(), fieldWidth);
    auto tests{student.getTests()};
    if (!tests.empty()) {
      for (const auto& test : tests) {
        os << std::format("{} : {:3} |", test.testName, test.grade);
      }
    }
    // separator
    os << "\n" << std::string((fieldWidth * 2) + (rowLength * 6), '-') << "\n";
  }
  return os;
}

std::optional<const AA::Student*> AA::StudentSingleton::find(
    const Student& student) const {
  auto pos = m_students.find(student);
  if (pos == m_students.end()) {
    return std::nullopt;
  }

  return &(*pos);
}

std::optional<AA::Student*> AA::StudentSingleton::find(const Student& student) {
  auto found = std::as_const(*this).find(student);
  if (found == std::nullopt) {
    return std::nullopt;
  }
  auto value = found.value();
  return const_cast<AA::Student*>(value);
}
