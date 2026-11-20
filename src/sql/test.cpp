#include "sql/test.h"

#include <mysql++/mysql++.h>

#include "Date.h"
#include "OpenTest.h"

bool AA::Data::test::saveOpenTest(const AA::OpenTest& openTest) {
  if (!m_conn.connected()) return false;

  try {
    mysqlpp::Query query = m_conn.query();
    query << "INSERT INTO open_tests (test_id, teacher_id, creation_date, "
             "due_date, duration) "
          << "VALUES (" << mysqlpp::quote << openTest.getTestID() << ", "
          << mysqlpp::quote << openTest.getTeacherID() << ", " << mysqlpp::quote
          << openTest.getCreationDate().toStringSql() << ", " << mysqlpp::quote
          << openTest.getDueDate().toStringSql() << ", "
          << openTest.getDuration() << ")";

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to save open test: ") +
                       query.error());
      return false;
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception saving open test: ") + e.what());
    return false;
  }
}

bool AA::Data::test::loadOpenTestsByDateRange(
    const DT::Date& startDate, const DT::Date& endDate,
    std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();

    // Construct the query using BETWEEN with safely quoted DT::Date strings
    query
        << "SELECT id, test_id, teacher_id, creation_date, due_date, duration "
        << "FROM open_tests "
        << "WHERE due_date BETWEEN " << mysqlpp::quote
        << startDate.toStringSql() << " AND " << mysqlpp::quote
        << endDate.toStringSql() << " ORDER BY due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    // Map the result rows back into your OpenTestRecord structs
    for (size_t i = 0; i < res.num_rows(); ++i) {
      AA::OpenTest record;
      record.setID(res[i]["id"]);
      record.setTestID(res[i]["test_id"].c_str());
      record.setTeacherID(res[i]["teacher_id"].c_str());
      record.setCreationDate(DT::Date(res[i]["creation_date"].c_str()));
      record.setDueDate(DT::Date(res[i]["due_date"].c_str()));
      record.setDuration(res[i]["duration"]);

      out_tests.push_back(record);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests by date range: ") +
                     e.what());
    return false;
  }
}

// this allow getting only so many test at a time
bool AA::Data::test::loadTestsPaginated(int limit, int offset,
                                        std::vector<AA::Test>& out_tests) {
  out_tests.clear();

  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load paginated "
        "tests.");
    return false;
  }

  try {
    // Construct the query with LIMIT and OFFSET for pagination
    mysqlpp::Query query = m_conn.query();
    query << "SELECT t.test_id, t.name, t.subject, t.description, "
             "t.creation_date "
          << "FROM tests t "
          << "ORDER BY t.creation_date DESC "
          << "LIMIT " << limit << " OFFSET " << offset;

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    // Loop through the results and build your Test objects
    for (size_t i = 0; i < res.num_rows(); ++i) {
      AA::Test record;
      record.setTestID(res[i]["test_id"].c_str());
      // Set other fields here (name, subject, description, creation_date, etc.)
      record.setTestID(res[i]["test_id"].c_str());

      record.setTestName(res[i]["name"].c_str());

      record.setSubject(res[i]["subject"].c_str());

      record.setDescription(res[i]["description"].c_str());
      record.setSentDate(DT::Date{res[i]["sent_date"].c_str()});
      out_tests.push_back(record);
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading paginated tests: ") +
                     e.what());
    return false;
  }
}

namespace {

bool insertTestQuestions(mysqlpp::Connection& conn, const std::string& testId,
                         const AA::TestVec& questions) {
  for (const auto& qa : questions) {
    auto row = qa.toSSQLS();
    row.test_id = testId;

    mysqlpp::Query query = conn.query();
    query.insert(row);

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed inserting test question: ") +
                       query.error());
      return false;
    }
  }
  return true;
}

}  // namespace

bool AA::Data::test::saveTestWithQuestions(const AA::Test& test) {
  if (!m_conn.connected() || test.getTestID().empty()) {
    return false;
  }

  mysqlpp::Transaction trans(m_conn);

  try {
    mysqlpp::Query testQuery = m_conn.query();
    testQuery.insert(test.toSSQLS());
    if (!testQuery.execute()) {
      AA::SaveErrorLog(std::string("Failed inserting test row: ") +
                       testQuery.error());
      return false;
    }

    if (!insertTestQuestions(m_conn, test.getTestID(), test.getTest())) {
      return false;
    }

    trans.commit();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception saving test with questions: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::test::loadTestQuestions(const std::string& testId,
                                       TestVec& out_questions) const {
  out_questions.clear();

  if (!m_conn.connected() || testId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT id, test_id, question, answer FROM test_questions "
          << "WHERE test_id = " << mysqlpp::quote << testId
          << " ORDER BY id ASC";

    std::vector<SSQLS::test_questions> rows;
    query.storein(rows);

    out_questions.reserve(rows.size());
    for (const auto& row : rows) {
      AA::QandA qa;
      qa.fromSSQLS(row);
      out_questions.push_back(std::move(qa));
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading test questions: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::test::removeTestQuestions(const std::string& testId) {
  if (!m_conn.connected() || testId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "DELETE FROM test_questions WHERE test_id = " << mysqlpp::quote
          << testId;

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed removing test questions: ") +
                       query.error());
      return false;
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception removing test questions: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::test::loadTestWithQuestions(const std::string& testId,
                                           AA::Test& out_test) const {
  out_test.clear();

  if (!m_conn.connected() || testId.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT t.*, "
          << "th.l_name, th.f_name, th.email_addr, "
          << "s.name AS school_name, s.address AS school_address, "
          << "s.gradebook AS school_gradebook "
          << "FROM tests t "
          << "JOIN teachers th ON t.teacher_id = th.teacher_id "
          << "JOIN schools s ON th.school_name = s.name "
          << "WHERE t.test_id = " << mysqlpp::quote << testId << " LIMIT 1";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res || res.num_rows() == 0) {
      return false;
    }

    AA::School school(res[0]["school_name"].c_str(),
                      res[0]["school_address"].c_str(),
                      res[0]["school_gradebook"].c_str());

    AA::Teacher teacher(res[0]["teacher_id"].c_str(), res[0]["l_name"].c_str(),
                        res[0]["f_name"].c_str(), res[0]["email_addr"].c_str(),
                        school);

    SSQLS::tests test_row;
    test_row.test_id = res[0]["test_id"].c_str();
    test_row.name = res[0]["name"].c_str();
    test_row.teacher_id = res[0]["teacher_id"].c_str();
    test_row.sent_date = res[0]["sent_date"].c_str();
    test_row.due_date = res[0]["due_date"].c_str();
    test_row.subject = res[0]["subject"].c_str();
    test_row.description = res[0]["description"].c_str();
    test_row.level = res[0]["level"];

    AA::Test loaded = AA::Test::fromSSQLS(test_row, teacher);
    loaded.setID(test_row.test_id);

    AA::TestVec questions;
    if (!loadTestQuestions(testId, questions)) {
      return false;
    }

    loaded.setTest(std::move(questions));
    out_test = std::move(loaded);
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading test with questions: ") +
                     e.what());
    return false;
  }
}
