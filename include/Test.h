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
#include "sql/SSQLS.h"

namespace AA {
class QandA {
 private:
  std::string m_question;
  std::string m_answer;
  std::size_t m_id;
  std::string m_testID;

 public:
  QandA() = default;
  QandA(std::string_view queation, std::string_view answer)
      : m_question{queation}, m_answer{answer}, m_id{0}, m_testID{""} {}

  const std::string& getQuestion() const { return m_question; }
  const std::string& getAnswer() const { return m_answer; }
  std::size_t getID() const { return m_id; }
  const std::string& getTestID() const { return m_testID; }

  void setQuestion(std::string_view question) { m_question = question; }
  void setAnswer(std::string_view answer) { m_answer = answer; }
  void setID(const std::size_t id) { m_id = id; }
  void setTestID(std::string_view id) { m_testID = id; }

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

  SSQLS::test_questions toSSQLS() const {
    SSQLS::test_questions row;
    row.id = m_id;
    row.test_id = m_testID;
    row.question = m_question;
    row.answer = m_answer;
    return row;
  }

  static QandA fromSSQLS(const SSQLS::test_questions& row) {
    QandA qa(row.question, row.answer);
    // If you store/expose ID fields in QandA, map them here:
    qa.setID(row.id);
    qa.setTestID(row.test_id);
    return qa;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(QandA, m_question, m_answer, m_id, m_testID)
};  // QandA

using TestVec = std::vector<QandA>;

class Test {
 private:
  class studentExt : public Student {
   private:
    std::size_t m_id;
    std::string m_testID;
    std::vector<std::string> m_returnTest;
    std::vector<std::size_t> m_wrongAnswers;
    float m_grade = 0.0f;
    DT::Date m_rcvDate;

   public:
    studentExt() = default;
    studentExt(const Student& s, std::size_t id) : Student(s), m_id{id} {}
    studentExt(const std::shared_ptr<Student>& s, std::size_t id)
        : Student(*s), m_id{id} {}
    studentExt(const std::shared_ptr<Student>& s) : Student(*s) {}
    studentExt(const Student& s) : Student(s) {}

    // getters
    const std::vector<std::string>& getReturnTest() const {
      return m_returnTest;
    }

    const std::vector<std::size_t>& getWrongAnswers() const {
      return m_wrongAnswers;
    }
    const float& getGrade() const { return m_grade; }

    const DT::Date& getRcvDate() const { return m_rcvDate; }

    const std::string& getTestID() const { return m_testID; }

    // setters
    void setWrongAnswer(const std::size_t num) {
      m_wrongAnswers.push_back(num);
    }

    void setGrade(const float grade) { m_grade = grade; }
    void setReturnedTest(const std::vector<std::string>& test) {
      m_returnTest = test;
    }

    void setRcvDate(const DT::Date& date) { m_rcvDate = date; }

    void setTestID(std::string_view str) { m_testID = str; }
    void setID(const std::size_t id) { m_id = id; }

    std::strong_ordering operator<=>(const studentExt& other) const {
      return static_cast<const Student&>(*this) <=>
             static_cast<const Student&>(other);
    }

    bool operator==(const studentExt& other) const {
      return static_cast<const Student&>(*this) ==
             static_cast<const Student&>(other);
    }

    SSQLS::student_tests toSSQLS() const {
      SSQLS::student_tests row;
      row.id = m_id;
      row.test_id = m_testID;
      row.student_id = getStudentID();

      // 1. Convert std::vector<std::string> to JSON array, then dump to string
      nlohmann::json j_returnTest = m_returnTest;
      row.return_test = j_returnTest.dump();

      // 2. Convert std::vector<std::size_t> to JSON array, then dump to string
      nlohmann::json j_wrongAnswers = m_wrongAnswers;
      row.wrong_answers = j_wrongAnswers.dump();

      row.grade = m_grade;
      row.rcv_date = m_rcvDate.toStringSql();

      return row;
    }

    static studentExt fromSSQLS(const SSQLS::student_tests& row,
                                const Student& student) {
      studentExt ext(student, row.id);
      ext.setTestID(row.test_id);
      ext.setGrade(row.grade);
      ext.setRcvDate(DT::Date(row.rcv_date));

      // 1. Parse return_test JSON vector (std::vector<std::string>)
      if (!row.return_test.empty()) {
        nlohmann::json j_ret =
            nlohmann::json::parse(row.return_test, nullptr, false);
        if (!j_ret.is_discarded() && j_ret.is_array()) {
          ext.setReturnedTest(j_ret.get<std::vector<std::string>>());
        }
      }

      // 2. Parse wrong_answers JSON vector (std::vector<std::size_t>)
      if (!row.wrong_answers.empty()) {
        nlohmann::json j_wrong =
            nlohmann::json::parse(row.wrong_answers, nullptr, false);
        if (!j_wrong.is_discarded() && j_wrong.is_array()) {
          for (std::size_t index : j_wrong.get<std::vector<std::size_t>>()) {
            ext.setWrongAnswer(index);
          }
        }
      }

      return ext;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(studentExt, m_returnTest, m_wrongAnswers,
                                   m_grade, m_testID, m_id)
  };  // studentExt
  std::string m_name;
  Teacher m_teacher;
  DT::Date m_sentDate;
  DT::Date m_dueDate;
  TestVec m_test;
  std::string m_subject;
  std::string m_testID;
  std::string m_description;
  std::set<studentExt> m_students;
  std::size_t m_level = 0;

  static inline auto studentLess = [](const std::shared_ptr<Student>& p1,
                                      const std::shared_ptr<Student>& p2) {
    if (!p1) return p2 != nullptr;
    if (!p2) return false;
    return *p1 < *p2;
  };

  static std::string generateID();

 public:
  Test() = default;
  Test(std::string_view testName, const Teacher& teacher, std::string_view id,
       const std::size_t level)
      : m_name{testName},
        m_teacher{teacher},
        m_testID{(id.empty()) ? generateID() : id},
        m_level{level} {}
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

  std::optional<DT::Date> getRcvDate(
      const std::shared_ptr<Student>& student) const;

  float getGrade(const std::shared_ptr<Student>& student) const;

  std::optional<std::vector<std::size_t>> getWrongAnswers(
      const std::shared_ptr<Student>& student) const;
  const std::string& getTestID() const { return m_testID; }
  // setters
  void setDueDate(const DT::Date& date) { m_dueDate = date; }
  void setSentDate(const DT::Date& date) { m_sentDate = date; }
  void setTestName(std::string_view name) { m_name = name; }
  void setTeacher(const Teacher& teacher) { m_teacher = teacher; }
  void setID(std::string_view id) { m_testID = id; }
  void setDescription(std::string_view descr) { m_description = descr; }
  void setSubject(std::string_view subj) { m_subject = subj; }
  void setLevel(const std::size_t level) { m_level = level; }
  void setTestID(std::string_view id) { m_testID = id; }
  void setTest(const TestVec& tVec) { m_test = tVec; }
  bool setStudentTest(const std::shared_ptr<Student>& student,
                      const std::vector<std::string>& test,
                      const DT::Date& date, std::size_t id = 0);
  // add and remove students
  void addStudent(const std::shared_ptr<Student>& student,
                  const std::size_t id = 0);
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
  // (earliest due date first)
  std::strong_ordering operator<=>(const Test& rhs) const {
    return rhs.m_dueDate <=> m_dueDate;
  }
  bool operator==(const Test& rhs) const {
    return m_name == rhs.m_name && m_level == rhs.m_level &&
           m_subject == rhs.m_subject;
  }
  operator studentGradeMap() const;

  SSQLS::tests toSSQLS() const {
    SSQLS::tests row;
    row.test_id = m_testID;
    row.name = m_name;
    row.teacher_id =
        m_teacher.getTeacherID();  // Extract foreign key from Teacher object
    row.sent_date = m_sentDate.toStringSql();
    row.due_date = m_dueDate.toStringSql();
    row.subject = m_subject;
    row.description = m_description;
    row.level = static_cast<unsigned int>(m_level);
    return row;
  }

  static Test fromSSQLS(const SSQLS::tests& row, const Teacher& teacher) {
    Test test;
    test.setTestName(row.name);
    test.setTeacher(teacher);
    test.setSentDate(DT::Date(row.sent_date));
    test.setDueDate(DT::Date(row.due_date));
    test.setSubject(row.subject);
    test.setDescription(row.description);
    test.setLevel(static_cast<std::size_t>(row.level));
    // Note: test_id and questions/students are populated via repository joins
    return test;
  }

  void fromSSQLS(const mysqlpp::Row& row) {
    SSQLS::tests ssqlsRow(row);
    Teacher defaultTeacher{};
    *this = fromSSQLS(ssqlsRow, defaultTeacher);
    if (!ssqlsRow.test_id.empty()) {
      setTestID(ssqlsRow.test_id);
    }
  }

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
