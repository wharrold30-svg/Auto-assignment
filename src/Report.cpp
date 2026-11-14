#include "Report.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <format>
#include <string>
#include <vector>

AA::Report::Report(const Test& test) : AA::Report{} {
  const auto foundStudents{test.getStudents()};
  if (!foundStudents) {
    return;
  }
  for (const auto students{foundStudents.value()};
       const auto& [student, grade] : students) {
    m_data.insert({student->getName(), grade});
  }
}

AA::GradeMap AA::Report::five(
    std::function<void(std::span<DataIter>)> partial_sort_fn) const {
  // 1. Collect lightweight iterators in a single O(N) pass
  std::vector<DataIter> iters;
  iters.reserve(m_data.size());
  for (auto it = m_data.cbegin(); it != m_data.cend(); ++it) {
    iters.push_back(it);
  }

  // 2. Partially sort only the top/bottom 5 iterators
  partial_sort_fn(iters);

  // 3. Build the resulting GradesMap directly from the first 5 iterators
  GradeMap gm;
  const size_t count = std::min<size_t>(5, iters.size());
  for (size_t i = 0; i < count; ++i) {
    // iters[i]->second is the grade (float)
    // iters[i]->first is the name/data (std::string_view)
    gm.emplace(iters[i]->second, iters[i]->first);
  }
  return gm;
}

AA::GradeMap AA::Report::topFive() const {
  return five([](std::span<DataIter> iters) {
    const size_t k = std::min<size_t>(5, iters.size());
    std::ranges::partial_sort(
        iters, iters.begin() + k, std::greater<>{},
        [](DataIter it) { return it->second; });  // Compare by grade
  });
}

AA::GradeMap AA::Report::bottomFive() const {
  return five([](std::span<DataIter> iters) {
    const size_t k = std::min<size_t>(5, iters.size());
    std::ranges::partial_sort(
        iters, iters.begin() + k, std::less<>{},
        [](DataIter it) { return it->second; });  // Compare by grade
  });
}

float AA::Report::mean() const {
  float sum{0.0f};
  for (const auto& [name, grade] : m_data) {
    sum += grade;
  }
  return sum / m_data.size();
}

float AA::Report::meadian() const {
  std::vector<float> grades;
  grades.reserve(m_data.size());
  for (const auto& [name, grade] : m_data) {
    grades.push_back(grade);
  }
  std::sort(std::execution::par, grades.begin(), grades.end());
  size_t n = grades.size();
  if (n % 2 == 1) {
    return grades[n / 2];
  } else {
    return (grades[(n / 2) - 1] + grades[n / 2]) / 2.0f;
  }
}

float AA::Report::var() const {
  const float mn{mean()};
  float deviation{0.0f};
  for (const auto& [name, grade] : m_data) {
    deviation += square(grade - mn);
  }
  return deviation / m_data.size();
}

float AA::Report::sd() const { return std::sqrt(var()); }

float AA::Report::percentage(std::function<bool(const float)> func) const {
  std::size_t num{0};
  for (const auto& [name, grade] : m_data) {
    if (func(grade)) {
      ++num;
    }
  }
  return (static_cast<float>(num) / m_data.size()) * 100;
}

float AA::Report::passPercentage() const {
  auto pass{[](const float grade) -> bool { return grade >= 60.0; }};
  return percentage(pass);
}
float AA::Report::failPercentage() const {
  auto fail{[](const float grade) -> bool { return grade < 60.0; }};
  return percentage(fail);
}

std::size_t AA::maxNameLength(const GradeMap& grades) {
  std::size_t max{0};
  for (const auto& [grade, name] : grades) {
    max = std::max(name.size(), max);
  }
  return max;
}

std::ostream& operator<<(std::ostream& os, const AA::Report& report) {
  auto show{[&os](const AA::GradeMap& grades) -> void {
    const std::size_t fieldWidth{AA::maxNameLength(grades) + 2};
    for (const auto& [grade, name] : grades) {
      os << std::format("{:{}} | {:3}\n", name, fieldWidth, grade);
      // separator
      os << std::string(fieldWidth * 2 + 6, '-');
      os << "\n";
    }
  }};

  os << "Top Five Grades:\n";
  show(report.topFive());

  os << "Bottom Five Grades:\n";
  show(report.bottomFive());

  os << std::format(
      "Mean: {:6.2f} | Median: {:6.2f} | Var: {:6.2f} | SD: {:6.2f}\n",
      report.mean(), report.meadian(), report.var(), report.sd());

  os << std::format("Passed: {:.2f}%\n", report.passPercentage());
  os << std::format("Failed: {:.2f}%\n", report.failPercentage());

  return os;
}

std::string AA::buildTeacherReport(const Test& test) {
  std::ostringstream out;
  const auto& teacher = test.getTeacher();
  std::string teacher_email = teacher.getEmailAddr();
  std::string teacher_name = teacher.getName();

  std::string teacher_subject = "Test Report & Grades: " + test.getTestName();
  std::string teacher_body = std::format(
      "Hello {},\n\n"
      "The deadline for test \"{}\" (ID: {}) has been reached.\n"
      "All submitted student tests have been automatically graded.\n\n"
      "Subject: {}\n"
      "Description: {}\n\n"
      "Best regards,\n"
      "Auto-Assignment System",
      teacher_name, test.getTestName(), test.getTestID(), test.getSubject(),
      test.getDescription());
  out << teacher_subject << "\n";
  out << teacher_body << "\n";
  Report report{test};
  out << test << "\n" << report;
  return out.str();
}

std::string AA::buildStundentReport(const std::shared_ptr<Student>& pStudent,
                                    const Test& test) {
  std::ostringstream out;
  std::string student_email = pStudent->getEmailAddr();
  std::string student_name = pStudent->getName();
  float student_grade = test.getGrade(pStudent);

  std::string student_subject = "Your Grade Report: " + test.getTestName();
  std::string student_body = std::format(
      "Hello {},\n\n"
      "Your test \"{}\" given on: {} has been graded.\n"
      "Your Final Grade: {:.2f}%\n\n"
      "Keep up the hard work!\n\n"
      "Best regards,\n"
      "{}",
      student_name, test.getTestName(), test.getSentDate().toStringWithDay(),
      student_grade, test.getTeacher().getName());

  auto foundTest{test.getStudentTest(pStudent)};
  if (foundTest.empty()) {
    out << "No test returned";
    return out.str();
  }
  out << "Turned in on: " << test.getRcvDate(pStudent).value().toStringWithDay()
      << "\n";
  const auto& rcvTest{test.getStudentTest(pStudent)};
  const auto& origTest{test.getTest()};
  auto foundWA{test.getWrongAnswers(pStudent)};
  auto wrongAns{foundWA.value_or(std::vector<std::size_t>{})};
  if (!wrongAns.empty()) {
    out << "Wrong Answers: ";
    for (const auto& wa : wrongAns) {
      out << wa << " ";
    }
    out << "\n";
  }

  out << "Questions and Answers:\n";
  auto rcvIt{rcvTest.cbegin()};
  for (std::size_t i{0}, end{origTest.size()}; i < end; ++i) {
    if (!wrongAns.empty() &&
        std::find(wrongAns.begin(), wrongAns.end(), i) != wrongAns.end()) {
      out << std::format("X {:3}) {}\n", i + 1, origTest[i].getQuestion());
      out << std::format(" {}\n", *rcvIt);
      out << "Correct answer:\n";
      out << std::format(" {}\n\n", origTest[i].getAnswer());
    } else {
      out << std::format(" {:3}) {}\n", i + 1, origTest[i].getQuestion());
      out << std::format(" {}\n\n", *rcvIt);
    }
    ++rcvIt;
  }
  return out.str();
}
