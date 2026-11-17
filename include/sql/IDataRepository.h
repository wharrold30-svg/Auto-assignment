#ifndef IDATA_REPOSITORY_H
#define IDATA_REPOSITORY_H

#include <string>

#include "Student.h"
#include "Test.h"

namespace AA {
class IDataRepository {
 public:
  virtual ~IDataRepository() = default;

  virtual bool saveStudent(const Student& student) = 0;
  virtual bool loadStudent(const std::string& student_id,
                           Student& out_student) = 0;

  virtual bool saveTeacher(const AA::Teacher& teacher) = 0;
  virtual bool loadTeacher(const std::string& teacher_id,
                           AA::Teacher& out_teacher) = 0;
  virtual bool saveTest(const Test& test) = 0;
  virtual bool loadTest(const std::string& test_id, Test& out_test) = 0;

  virtual bool saveSchool(const AA::School& school) = 0;

  virtual bool loadSchool(const std::string& school_name,
                          AA::School& out_school) = 0;

  virtual bool enrollStudentInClass(const std::string& teacherId,
                                    const std::string& studentId,
                                    unsigned int classPeriod) = 0;

  virtual bool saveTeacherRoster(
      const std::string& teacherId,
      const std::vector<std::pair<std::string, unsigned int>>&
          studentPeriodPairs) = 0;
};
}  // namespace AA

#endif
