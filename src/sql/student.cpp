#include "sql/student.h"

#include <string>

#include "GradeRecord.h"
#include "Log.h"

bool AA::Data::student::loadStudentSchedule(
    const std::string& studentId,
    std::vector<std::tuple<std::string, unsigned int>>& out_schedule) {
  if (!m_conn.connected()) return false;

  out_schedule.clear();
  try {
    // student_schedule view contains: id, student_id, teacher_id, class_period
    mysqlpp::Query query = m_conn.query();
    query << "SELECT teacher_id, class_period FROM student_schedule WHERE "
             "student_id = "
          << mysqlpp::quote << studentId << " ORDER BY class_period ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      std::string teacherId = res[i]["teacher_id"].c_str();
      unsigned int period = res[i]["class_period"];
      out_schedule.emplace_back(teacherId, period);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading student schedule: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::student::loadOpenTestsForStudent(
    const std::string& studentId, std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    // Join the student's log table with the open_tests table
    query << "SELECT ot.id, ot.test_id, ot.teacher_id, ot.creation_date, "
             "ot.due_date, ot.duration "
          << "FROM student_open_test_log sotl "
          << "JOIN open_tests ot ON sotl.open_test_id = ot.id "
          << "WHERE sotl.student_id = " << mysqlpp::quote << studentId
          << " ORDER BY ot.due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      int id = res[i]["id"];
      std::string testID = res[i]["test_id"].c_str();
      std::string tID = res[i]["teacher_id"].c_str();

      DT::Date creationDate(res[i]["creation_date"].c_str());
      DT::Date dueDate(res[i]["due_date"].c_str());

      unsigned int duration = res[i]["duration"];

      std::string status = res[i]["status"].c_str();

      out_tests.emplace_back(id, testID, tID, creationDate, dueDate, duration,
                             status);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests for student: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::student::getStudentAssignments(
    const std::string& studentId, std::vector<GradeRecord>& out_assignments) {
  out_assignments.clear();

  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load student "
        "assignments.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query
        << "SELECT ot.test_id, ot.name AS test_name, ot.subject, ot.due_date, "
        << "       sot.submission_date, sot.score, sot.status "
        << "FROM student_open_test_log sot "
        << "JOIN open_tests ot ON sot.open_test_id = ot.test_id "
        << "WHERE sot.student_id = " << mysqlpp::quote << studentId
        << " ORDER BY ot.due_date ASC";  // Order by due date so
                                         // pending/upcoming tests are front and
                                         // center

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      GradeRecord record;
      record.setTestID(res[i]["test_id"].c_str());
      record.setTestName(res[i]["test_name"].c_str());
      record.setSubject(res[i]["subject"].c_str());

      // Due date is always present for open tests
      if (!res[i]["due_date"].is_null()) {
        record.setDueDate(DT::Date{res[i]["due_date"].c_str()});
      }

      // Submission date and score might be null if pending/unsubmitted
      if (!res[i]["submission_date"].is_null()) {
        record.setSubmissionDate(DT::Date{res[i]["submission_date"].c_str()});
      }
      if (!res[i]["score"].is_null()) {
        record.setGrade(res[i]["grade"]);
      }

      // e.g., "pending", "completed", "not turned in"
      record.setStatus(res[i]["status"].c_str());

      out_assignments.push_back(record);
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading student assignments: ") +
                     e.what());
    return false;
  }
}

namespace {

bool upsertStudentTestSubmission(mysqlpp::Connection& conn,
                                 const std::string& testId,
                                 const std::string& studentId,
                                 const std::string& returnTest,
                                 const std::string& wrongAnswers, float grade,
                                 const std::string& receivedDate) {
  mysqlpp::Query query = conn.query();
  query << "INSERT INTO student_tests "
        << "(test_id, student_id, return_test, wrong_answers, grade, rcv_date) "
        << "VALUES (" << mysqlpp::quote << testId << ", " << mysqlpp::quote
        << studentId << ", " << mysqlpp::quote << returnTest << ", "
        << mysqlpp::quote << wrongAnswers << ", " << grade << ", "
        << mysqlpp::quote << receivedDate << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "return_test = VALUES(return_test), "
        << "wrong_answers = VALUES(wrong_answers), "
        << "grade = VALUES(grade), "
        << "rcv_date = VALUES(rcv_date)";
  return query.execute();
}

}  // namespace

bool AA::Data::student::returnStudentTest(
    const std::string& studentId, int openTestId,
    const DT::Date& submissionDate, double score, const std::string& status,
    const std::string& returnTest, const std::string& wrongAnswers,
    float grade) {
  if (!m_conn.connected() || studentId.empty() || openTestId <= 0 ||
      status.empty()) {
    return false;
  }

  try {
    // Resolve test_id from open_tests so student_tests gets the FK test_id
    mysqlpp::Query lookupQuery = m_conn.query();
    lookupQuery << "SELECT test_id FROM open_tests WHERE id = " << openTestId
                << " LIMIT 1";
    mysqlpp::StoreQueryResult lookupRes = lookupQuery.store();
    if (!lookupRes || lookupRes.num_rows() == 0) {
      AA::SaveErrorLog("returnStudentTest failed: open_test_id not found.");
      return false;
    }
    const std::string testId = std::string(lookupRes[0]["test_id"].c_str());
    const std::string dateStr = submissionDate.toStringSql();
    const float effectiveGrade =
        (grade == 0.0f && score != 0.0) ? static_cast<float>(score) : grade;

    mysqlpp::Transaction trans(m_conn);

    mysqlpp::Query logQuery = m_conn.query();
    logQuery << "UPDATE student_open_test_log SET "
             << "status = " << mysqlpp::quote << status << ", "
             << "submission_date = " << mysqlpp::quote << dateStr << ", "
             << "score = " << score << " "
             << "WHERE student_id = " << mysqlpp::quote << studentId
             << " AND open_test_id = " << openTestId;

    if (!logQuery.execute()) {
      AA::SaveErrorLog(std::string("Failed updating student_open_test_log: ") +
                       logQuery.error());
      return false;
    }

    if (!upsertStudentTestSubmission(m_conn, testId, studentId, returnTest,
                                     wrongAnswers, effectiveGrade, dateStr)) {
      AA::SaveErrorLog("Failed upserting student_tests submission.");
      return false;
    }

    trans.commit();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception in returnStudentTest: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::student::loadStudentByEmail(const std::string& email,
                                           AA::Student& out_student) const {
  if (!m_conn.connected() || email.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT student_id, email_addr, l_name, f_name, class_period, "
             "school_name, tests_json "
          << "FROM students WHERE email_addr = " << mysqlpp::quote << email
          << " LIMIT 1";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res || res.num_rows() == 0) {
      return false;
    }

    std::vector<SSQLS::students> rows;
    query.storein(rows);
    // If storein isn't used on a single-row result, map or pass `res[0]`
    // according to `AA::Student::fromSSQLS`'s parameter type:
    if (rows.empty()) {
      return false;
    }
    out_student.fromSSQLS(res[0]);
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("loadStudentByEmail failed: ") + e.what());
    return false;
  }
}
