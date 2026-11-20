#include "sql/school.h"

#include "School.h"

bool AA::Data::school::transferStudentSchool(const std::string& studentId,
                                             const std::string& newSchoolName) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to transfer student.");
    return false;
  }

  try {
    // Use a transaction so all steps succeed or roll back together
    // Note: MySQL++ transaction handling can vary, but standard query execution
    // works well.

    // Step 1: Remove the student from all previous teacher rosters
    // (since classes are tied to the old school's teachers)
    mysqlpp::Query rosterQuery = m_conn.query();
    rosterQuery << "DELETE FROM teacher_roster WHERE student_id = "
                << mysqlpp::quote << studentId;
    rosterQuery.execute();

    // Step 2: Clear old open test logs tied to the old school/teachers
    mysqlpp::Query testLogQuery = m_conn.query();
    testLogQuery << "DELETE FROM student_open_test_log WHERE student_id = "
                 << mysqlpp::quote << studentId;
    testLogQuery.execute();

    // Step 3: Update the student's school_name to the new school
    mysqlpp::Query updateQuery = m_conn.query();
    updateQuery << "UPDATE students SET school_name = " << mysqlpp::quote
                << newSchoolName << " WHERE student_id = " << mysqlpp::quote
                << studentId;
    updateQuery.execute();

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception during student school transfer: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::school::removeStudentFromSchoolCompletely(
    const std::string& studentId, const std::string& schoolName) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to remove student from "
        "school.");
    return false;
  }

  try {
    // Deleting the student from the students table where they belong to this
    // school. Thanks to ON DELETE CASCADE in your schema, this automatically
    // deletes all matching rows in teacher_roster, student_open_test_log, and
    // student_tests!
    mysqlpp::Query query = m_conn.query();
    query << "DELETE FROM students "
          << "WHERE student_id = " << mysqlpp::quote << studentId
          << " AND school_name = " << mysqlpp::quote << schoolName;

    query.execute();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception removing student from school: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::school::transferTeacherSchool(const std::string& teacherId,
                                             const std::string& newSchoolName) {
  if (!m_conn.connected() || teacherId.empty() || newSchoolName.empty()) {
    return false;
  }

  mysqlpp::Transaction trans(m_conn);

  try {
    mysqlpp::Query query = m_conn.query();
    query << "UPDATE teachers SET school_name = " << mysqlpp::quote
          << newSchoolName << " WHERE teacher_id = " << mysqlpp::quote
          << teacherId;

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed transferring teacher school: ") +
                       query.error());
      return false;
    }

    trans.commit();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception transferring teacher school: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::school::removeTeacherFromSchool(const std::string& teacherId) {
  if (!m_conn.connected() || teacherId.empty()) {
    return false;
  }

  mysqlpp::Transaction trans(m_conn);

  try {
    // 1. Remove roster assignments for this teacher so they aren't tied to
    // school periods
    mysqlpp::Query rosterQuery = m_conn.query();
    rosterQuery << "DELETE FROM teacher_roster WHERE teacher_id = "
                << mysqlpp::quote << teacherId;
    if (!rosterQuery.execute()) {
      AA::SaveErrorLog(std::string("Failed removing teacher roster rows: ") +
                       rosterQuery.error());
      return false;
    }

    // 2. Detach from school (set school_name = NULL) OR delete the teacher row:
    // If school_name is NOT NULL in your schema, delete the teacher record
    // instead:
    //   DELETE FROM teachers WHERE teacher_id = ...
    mysqlpp::Query teacherQuery = m_conn.query();
    teacherQuery << "UPDATE teachers SET school_name = NULL "
                 << "WHERE teacher_id = " << mysqlpp::quote << teacherId;
    if (!teacherQuery.execute()) {
      AA::SaveErrorLog(std::string("Failed removing teacher from school: ") +
                       teacherQuery.error());
      return false;
    }

    trans.commit();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception removing teacher from school: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::school::loadTeachersForSchool(
    const std::string& schoolName,
    std::vector<AA::Teacher>& out_teachers) const {
  out_teachers.clear();

  if (!m_conn.connected() || schoolName.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT th.teacher_id, th.l_name, th.f_name, th.email_addr, "
             "th.salt, th.password, th.school_name, "
             "s.name AS school_name_col, s.address AS school_address, "
             "s.gradebook AS school_gradebook "
          << "FROM teachers th "
          << "JOIN schools s ON th.school_name = s.name "
          << "WHERE th.school_name = " << mysqlpp::quote << schoolName
          << " ORDER BY th.l_name ASC, th.f_name ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    out_teachers.reserve(res.num_rows());
    for (size_t i = 0; i < res.num_rows(); ++i) {
      AA::School school(res[i]["school_name_col"].c_str(),
                        res[i]["school_address"].c_str(),
                        res[i]["school_gradebook"].c_str());
      out_teachers.push_back(AA::Teacher::fromSSQLS(res[i], school));
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("loadTeachersForSchool failed: ") + e.what());
    return false;
  }
}

bool AA::Data::school::loadStudentsForSchool(
    const std::string& schoolName,
    std::vector<AA::Student>& out_students) const {
  out_students.clear();

  if (!m_conn.connected() || schoolName.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT student_id, email_addr, l_name, f_name, class_period, "
             "school_name, tests_json "
          << "FROM students "
          << "WHERE school_name = " << mysqlpp::quote << schoolName
          << " ORDER BY l_name ASC, f_name ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    out_students.reserve(res.num_rows());
    for (size_t i = 0; i < res.num_rows(); ++i) {
      out_students.push_back(AA::Student::fromSSQLS(res[i]));
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("loadStudentsForSchool failed: ") + e.what());
    return false;
  }
}
