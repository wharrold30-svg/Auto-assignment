#include "sql/teacher.h"

#include <mysql++/mysql++.h>

#include <string>
#include <vector>

#include "OpenTest.h"
#include "Teacher.h"
#include "Test.h"

bool AA::Data::teacher::loadTestsForTeacher(const std::string& teacherId,
                                            std::vector<AA::Test>& out_tests) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load teacher tests.");
    return false;
  }

  out_tests.clear();
  try {
    // 1. Join tests, teachers, and schools tables to fetch all fields for the
    // entire collection
    mysqlpp::Query query = m_conn.query();
    query << "SELECT t.*, "
          << "th.l_name, th.f_name, th.email_addr, "
          << "s.name AS school_name, s.address AS school_address, s.gradebook "
             "AS school_gradebook "
          << "FROM tests t "
          << "JOIN teachers th ON t.teacher_id = th.teacher_id "
          << "JOIN schools s ON th.school_name = s.name "
          << "WHERE t.teacher_id = " << mysqlpp::quote << teacherId
          << " ORDER BY t.due_date DESC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) return false;

    for (size_t i = 0; i < res.num_rows(); ++i) {
      // 2. Reconstruct School object
      AA::School school(res[i]["school_name"].c_str(),
                        res[i]["school_address"].c_str(),
                        res[i]["school_gradebook"].c_str());

      // 3. Reconstruct Teacher object
      AA::Teacher teacher(res[i]["teacher_id"].c_str(),
                          res[i]["l_name"].c_str(), res[i]["f_name"].c_str(),
                          res[i]["email_addr"].c_str(), school);

      // 4. Map query row to SSQLS::tests
      SSQLS::tests test_row;
      test_row.test_id = res[i]["test_id"].c_str();
      test_row.name = res[i]["name"].c_str();
      test_row.teacher_id = res[i]["teacher_id"].c_str();
      test_row.sent_date = res[i]["sent_date"].c_str();
      test_row.due_date = res[i]["due_date"].c_str();
      test_row.subject = res[i]["subject"].c_str();
      test_row.description = res[i]["description"].c_str();
      test_row.level = res[i]["level"];

      // 5. Use your static fromSSQLS factory method to instantiate Test
      AA::Test test = AA::Test::fromSSQLS(test_row, teacher);
      test.setID(test_row.test_id);

      // 6. Load and attach all associated questions/answers from test_questions
      // table
      mysqlpp::Query q_query = m_conn.query();
      q_query << "SELECT id, test_id, question, answer FROM test_questions "
              << "WHERE test_id = " << mysqlpp::quote << test_row.test_id
              << " ORDER BY id ASC";

      std::vector<SSQLS::test_questions> q_rows;
      q_query.storein(q_rows);

      TestVec questions;
      for (const auto& q_row : q_rows) {
        questions.push_back(AA::QandA::fromSSQLS(q_row));
      }
      test.setTest(questions);

      // Add fully populated test to the output vector
      out_tests.push_back(test);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading tests for teacher: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::teacher::assignOpenTestToMultipleStudents(
    const std::vector<std::string>& studentIds, int openTestId) {
  if (!m_conn.connected()) return false;

  mysqlpp::Transaction trans(m_conn);

  try {
    for (const auto& studentId : studentIds) {
      mysqlpp::Query query = m_conn.query();
      query << "INSERT INTO student_open_test_log (student_id, open_test_id) "
            << "VALUES (" << mysqlpp::quote << studentId << ", " << openTestId
            << ") "
            << "ON DUPLICATE KEY UPDATE "
            << "open_test_id = VALUES(open_test_id)";

      if (!query.execute()) {
        return false;  // Transaction automatically rolls back on scope exit if
                       // not committed
      }
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Batch open test assignment failed: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::teacher::loadOpenTestsForTeacher(
    const std::string& teacherId, std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    query
        << "SELECT id, test_id, teacher_id, creation_date, due_date, duration "
        << "FROM open_tests WHERE teacher_id = " << mysqlpp::quote << teacherId
        << " ORDER BY due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      int id = res[i]["id"];
      std::string testID = res[i]["test_id"].c_str();
      std::string tID = res[i]["teacher_id"].c_str();

      // Parse database strings into DT::Date
      DT::Date creationDate(res[i]["creation_date"].c_str());
      DT::Date dueDate(res[i]["due_date"].c_str());

      unsigned int duration = res[i]["duration"];

      std::string status = res[i]["status"].c_str();

      // Construct using your OpenTest constructor
      out_tests.emplace_back(id, testID, tID, creationDate, dueDate, duration,
                             status);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests for teacher: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::teacher::loadTeacherRoster(
    const std::string& teacherId,
    std::vector<std::pair<std::string, unsigned int>>& out_roster) {
  if (!m_conn.connected()) return false;

  out_roster.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT student_id, class_period FROM teacher_roster WHERE "
             "teacher_id = "
          << mysqlpp::quote << teacherId << " ORDER BY class_period ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      std::string studentId = res[i]["student_id"].c_str();
      unsigned int period = res[i]["class_period"];
      out_roster.emplace_back(studentId, period);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading teacher roster: ") + e.what());
    return false;
  }
}

bool AA::Data::teacher::saveTeacherRoster(
    const std::string& teacherId,
    const std::vector<std::pair<std::string, unsigned int>>&
        studentPeriodPairs) {
  if (!m_conn.connected()) return false;

  mysqlpp::Transaction trans(m_conn);

  try {
    for (const auto& [studentId, period] : studentPeriodPairs) {
      mysqlpp::Query query = m_conn.query();
      query << "INSERT INTO teacher_roster (teacher_id, student_id, "
               "class_period) "
            << "VALUES (" << mysqlpp::quote << teacherId << ", "
            << mysqlpp::quote << studentId << ", " << period << ") "
            << "ON DUPLICATE KEY UPDATE "
            << "class_period = VALUES(class_period)";

      if (!query.execute()) {
        // Rollback occurs automatically if transaction is not committed
        return false;
      }
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Batch roster save failed: ") + e.what());
    return false;
  }
}

bool AA::Data::teacher::loadCredentials(const std::string& teacherId,
                                        std::string& out_salt,
                                        std::string& out_password) {
  if (!m_conn.connected() || teacherId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT salt, password FROM teachers WHERE teacher_id = "
          << mysqlpp::quote << teacherId;

    mysqlpp::StoreQueryResult res = query.store();
    if (!res || res.num_rows() == 0) {
      return false;
    }

    out_salt = res[0]["salt"].c_str();
    out_password = res[0]["password"].c_str();
    return true;
  } catch (const mysqlpp::Exception& e) {
    return false;
  }
}

bool AA::Data::teacher::saveCredentials(const std::string& teacherId,
                                        const std::string& salt,
                                        const std::string& password) {
  if (!m_conn.connected() || teacherId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "UPDATE teachers SET "
          << "salt = " << mysqlpp::quote << salt << ", "
          << "password = " << mysqlpp::quote << password
          << " WHERE teacher_id = " << mysqlpp::quote << teacherId;

    query.execute();
    return true;
  } catch (const mysqlpp::Exception& e) {
    return false;
  }
}

bool AA::Data::teacher::enrollStudentInClass(const std::string& teacherId,
                                             const std::string& studentId,
                                             unsigned int classPeriod) {
  if (!m_conn.connected()) return false;

  // Use a transaction to ensure foreign key lookups and inserts are safe
  mysqlpp::Transaction trans(m_conn);

  try {
    mysqlpp::Query query = m_conn.query();

    // Insert or update the enrollment mapping
    query
        << "INSERT INTO teacher_roster (teacher_id, student_id, class_period) "
        << "VALUES (" << mysqlpp::quote << teacherId << ", " << mysqlpp::quote
        << studentId << ", " << classPeriod
        << ") "  // Numbers don't require mysqlpp::quote
        << "ON DUPLICATE KEY UPDATE "
        << "class_period = VALUES(class_period)";

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to save student enrollment: ") +
                       query.error());
      return false;
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception saving student enrollment: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::teacher::removeStudentFromClass(const std::string& teacherId,
                                               const std::string& studentId,
                                               unsigned int classPeriod) {
  if (!m_conn.connected() || teacherId.empty() || studentId.empty()) {
    return false;
  }
  try {
    mysqlpp::Query query = m_conn.query();
    query << "DELETE FROM teacher_roster WHERE teacher_id = " << mysqlpp::quote
          << teacherId << " AND student_id = " << mysqlpp::quote << studentId
          << " AND class_period = " << classPeriod;
    query.execute();
    return query.affected_rows() > 0;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("removeStudentFromClass failed: ") + e.what());
    return false;
  }
}

bool AA::Data::teacher::assignOpenTestToStudent(const std::string& studentId,
                                                int openTestId) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to assign open test.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();

    // Insert the relationship row.
    // student_id is a string (requires mysqlpp::quote), openTestId is an int
    // (does not).
    query
        << "INSERT INTO student_open_test_log (student_id, open_test_id) "
        << "VALUES (" << mysqlpp::quote << studentId << ", " << openTestId
        << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "open_test_id = VALUES(open_test_id)";  // No-op update to prevent
                                                   // errors if already assigned

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to assign open test to student: ") +
                       query.error());
      return false;
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(
        std::string("MySQL exception while assigning open test: ") + e.what());
    return false;
  }
}

bool AA::Data::teacher::loadTeacherByEmail(const std::string& email,
                                           AA::Teacher& out_teacher) const {
  if (!m_conn.connected() || email.empty()) {
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
          << "WHERE th.email_addr = " << mysqlpp::quote << email << " LIMIT 1";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res || res.num_rows() == 0) {
      return false;
    }

    // Use whichever overload/signature AA::Teacher::fromSSQLS defines:
    // e.g., AA::Teacher::fromSSQLS(res[0]) or AA::Teacher::fromSSQLS(row,
    // school)
    AA::School school(res[0]["school_name_col"].c_str(),
                      res[0]["school_address"].c_str(),
                      res[0]["school_gradebook"].c_str());
    out_teacher.fromSSQLS(res[0]);
    out_teacher.setSchool(school);
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("loadTeacherByEmail failed: ") + e.what());
    return false;
  }
}

bool AA::Data::teacher::loadRosterForPeriod(
    const std::string& teacherId, unsigned int classPeriod,
    std::vector<std::string>& out_studentIds) const {
  out_studentIds.clear();

  if (!m_conn.connected() || teacherId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT student_id FROM teacher_roster "
          << "WHERE teacher_id = " << mysqlpp::quote << teacherId
          << " AND class_period = " << classPeriod
          << " ORDER BY student_id ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    out_studentIds.reserve(res.num_rows());
    for (size_t i = 0; i < res.num_rows(); ++i) {
      out_studentIds.emplace_back(res[i]["student_id"].c_str());
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("loadRosterForPeriod failed: ") + e.what());
    return false;
  }
}
