#ifndef STUDENTSINGLETON_H
#define STUDENTSINGLETON_H

// thissingleton class handles all of the accessingof the students

#include <memory>
#include <set>
#include <vector>

#include "Student.h"

namespace AA {
class StudentSingleton {
 private:
  std::set<Student> m_students;

  void load();
  void load();

  StudentSingleton();
  ~StudentSingleton();

 public:
  static StudentSingleton& Instance();

  void addStudent(const Student& student);
  void addStudent(const std::set<Student>& students);
  void removeStudent(const Student& student);
  void removeStudent(const std::set<Student>& students);

  std::unique_ptr<std::set<Student>> getStudents() const {
    return std::make_unique<std::set<Student>>(m_students);
  }

  //

};  // StudentSingleton
}  // namespace AA
#endif
