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

  //[] used to get all the studentsbin a class
  // period, throws out_of_rang exception if not students are in that class
  // period
  const std::vector<std::unique_ptr<Student>> operator[](
      const std::size_t period) const;
  std::vector<std::unique_ptr<Student>> operator[](const std::size_t period);
  const Student& find(const Student& student) const;
  Student& find(const Student&);

  Students(const Students&) = delete;
  Students& operator=(const Students&) = delete;

  Students(Students&&) = delete;
  Students& operator=(const Students&&) = delete;

};  // StudentSingleton
std::size_t maxNameLength(const std::set<Student>& students);
}  // namespace AA

std::ostream& operator<<(std::ostream& os,
                         const std::set<AA::Student>& students);
#endif
