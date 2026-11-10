#ifndef TEST_H
#define TEST_H

#include <compare>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
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

  // for structured bindings
  template <size_t I>
  const std::string& get() const {
    static_assert(I < 2, "Index out of bounds for QandA decomposition.");
    if constexpr (I == 0) return m_question;
    if constexpr (I == 1) return m_answer;
  }

  template <size_t I>
  std::string& get() {
    static_assert(I < 2, "Index out of bounds for QandA decomposition.");
    if constexpr (I == 0) return m_question;
    if constexpr (I == 1) return m_answer;
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

    // getters
    const std::vector<std::string>& getReturnTest() const {
      return m_returnTest;
    }

    const std::vector<std::size_t>& getWrongAnswerd() const {
      return m_wrongAnswers;
    }
    const float& getGrade() const { return m_grade; }

    // setters
    void setWrongAnswer(const std::size_t num) {
      m_wrongAnswers.push_back(num);
    }

    void setGrade(const float grade) { m_grade = grade; }
    void setReturnedTest(const std::vector<std::string>& test) {
      m_returnTest = test;
    }

    std::strong_ordering operator<=>(const studentExt& other) const {
      return static_cast<const Student&>(*this) <=>
             static_cast<const Student&>(other);
    }

    bool operator==(const studentExt& other) const {
      return static_cast<const Student&>(*this) ==
             static_cast<const Student&>(other);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(studentExt, m_returnTest, m_wrongAnswers,
                                   m_grade)
  };  // studentExt
  std::string m_name;
  Teacher m_teacher;
  DT::Date m_sentDate;
  DT::Date m_dueDate;
  TestVec m_test;
  std::string m_subject;
  std::string m_description;
  std::set<studentExt> m_students;
  std::size_t m_level = 0;

  static inline auto studentLess = [](const std::shared_ptr<Student>& p1,
                                      const std::shared_ptr<Student>& p2) {
    if (!p1) return p2 != nullptr;
    if (!p2) return false;
    return *p1 < *p2;
  };

 public:
  Test() = default;
  Test(std::string_view testName, const Teacher& teacher,
       const std::size_t level)
      : m_name{testName}, m_teacher{teacher}, m_level{level} {}
  // getters
  const std::string& getTestName() const { return m_name; }
  const Teacher& getTeacher() const { return m_teacher; }
  std::size_t getLevel() const { return m_level; }

  const TestVec& getTest() const { return m_test; }
  const std::string& getSubject() const { return m_subject; }
  const std::string& getDescription() const { return m_description; }

  const DT::Date& getSentDate() const { return m_sentDate; }
  const DT::Date& getDueDate() const { return m_dueDate; }
  std::vector<std::string> getStudentTest(
      const std::shared_ptr<Student>& student) const;

  using studentGradeMap =
      std::map<std::shared_ptr<Student>, double, decltype(studentLess)>;
  std::optional<studentGradeMap> getStudents(
      const std::size_t period = 0) const;

  // setters
  void setDueDate(const DT::Date& date) { m_dueDate = date; }
  void setSentDate(const DT::Date& date) { m_sentDate = date; }
  void setTestName(std::string_view name) { m_name = name; }
  void setTeacher(const Teacher& teacher) { m_teacher = teacher; }
  void setDescription(std::string_view descr) { m_description = descr; }
  void setSubject(std::string_view subj) { m_subject = subj; }
  void setLevel(const std::size_t level) { m_level = level; }
  void setTest(const TestVec& tVec) { m_test = tVec; }
  bool setStudentTest(const std::shared_ptr<Student>& student,
                      const std::vector<std::string>& test);
  // add and remove students
  void addStudent(const std::shared_ptr<Student>& student);
  void removeStudent(const std::shared_ptr<Student>& student);

  void gradeTests();

  // checkbti see if a student has been given this test
  bool hasStudent(const std::shared_ptr<Student>& student) const;

  bool rcvTest(const std::shared_ptr<Student>& student) const;

  // clear
  void clear() { *this = Test{}; }

  // random returns a testVec with the questions and answers shuffled
  TestVec random() const;

  // space-ship operator overload so that teats
  // are sorted by due dates
  std::partial_ordering operator<=>(const Test& rhs) const {
    return m_dueDate <=> rhs.m_dueDate;
  }
  bool operator==(const Test& rhs) const {
    return m_name == rhs.m_name && m_level == rhs.m_level &&
           m_subject == rhs.m_subject;
  }
  operator studentGradeMap() const;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Test, m_name, m_teacher, m_dueDate, m_sentDate,
                                 m_test, m_students, m_description, m_subject,
                                 m_level)
};  // Test
std::size_t maxNameLength(const Test::studentGradeMap& students);

void saveTest(const Test& test);
Test loadTest(std::string_view testName);
}  // namespace AA
std::ostream& operator<<(std::ostream& os, const AA::Test& test);

// for structured bindings
namespace std {
template <>
struct tuple_size<AA::QandA> : std::integral_constant<std::size_t, 2> {};

template <std::size_t I>
struct tuple_element<I, AA::QandA> {
  static_assert(I < 2, "Index out of bounds for QandA template.");
  using type = std::string;
};
}  // namespace std
#endif
