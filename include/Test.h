#ifndef TEST_H
#define TEST_H

#include <compare>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Date.h"
#include "Student.h"
#include "Teacher.h"
#include "nlohmann/json.hpp"

namespace AA {
class QandA {
 private:
  std::string m_question;
  std::string m_answer;

 public:
  QandA() = default;
  QandA(std::string_view queation, std::string_view answer)
      : m_question{queation}, m_answer{answer} {}

  const std::string& getQuestion() const { return m_question; }
  const std::string& getAnswer() const { return m_answer; }

  void setQuestion(std::string_view question) { m_question = question; }
  void setAnswer(std::string_view answer) { m_answer = answer; }

  std::strong_ordering operator<=>(const QandA&) const = default;
  bool operator==(const QandA&) const = default;

  operator std::pair<std::string, std::string>() const {
    return std::make_pair(m_question, m_answer);
  }
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(QandA, m_question, m_answer)
};  // QandA

using TestVec = std::vector<QandA>;

class Test {
 private:
  class studentExt : public Student {
   private:
    std::vector<std::string> m_returnTest;
    std::vector<std::size_t> m_wrongAnswers;
    float m_grade = 0.0f;

   public:
    studentExt() = default;
    studentExt(const Student& s) : Student(s) {}
    studentExt(const std::shared_ptr<Student>& s) : Student(*s) {}

    const std::vector<std::string>& getReturnTest() const {
      return m_returnTest;
    }

    std::vector<std::string>& getReturnTest() { return m_returnTest; }

    const std::vector<std::size_t>& getWrongAnswerd() const {
      return m_wrongAnswers;
    }
    std::vector<std::size_t>& getWrongAnswerd() { return m_wrongAnswers; }
    const float& getGrade() const { return m_grade; }
    float& getGrade() { return m_grade; }

    using Student::operator<=>;
    using Student::operator==;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(studentExt, m_returnTest, m_wrongAnswers,
                                   m_grade)
  };
  std::string m_name;
  Teacher m_teacher;
  DT::Date m_sentDate;
  DT::Date m_dueDate;
  TestVec m_test;
  std::string m_subject;
  std::string m_description;
  std::set<studentExt> m_students;
  std::size_t m_level = 0;

 public:
  Test() = default;
  Test(std::string_view testName, const Teacher& teacher,
       const std::size_t level)
      : m_name{testName}, m_teacher{teacher}, m_level{level} {}
  // getters
  const std::string& getTestName() const { return m_name; }
  const Teacher& getTeacher() const { return m_teacher; }
  std::size_t getLevel() const { return m_level; }
};  // Test
}  // namespace AA
#endif
