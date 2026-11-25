#include "sql/gradebook.h"

#include <string>
#include <vector>

#include "Gradebook.h"
#include "Log.h"
#include "School.h"

namespace {

AA::School schoolFromSSQLS(const SSQLS::schools& row) {
  AA::School school;
  school.fromSSQLS(row);
  return school;
}

AA::Gradebook gradebookFromSchool(const AA::School& school) {
  const auto ssqlsRow = school.toSSQLS();
  // Replace with your AA::Gradebook constructor or parser if it parses
  // serialized data (e.g., JSON/CSV) rather than taking the raw string
  // directly.
  AA::Gradebook g;
  g.setName(std::string(ssqlsRow.gradebook.c_str()));
  return g;
}

}  // namespace

bool AA::Data::gradebook::loadAllGradeBooks(
    std::vector<std::pair<std::string, AA::Gradebook>>& out_gradeBooks) const {
  out_gradeBooks.clear();

  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while loading all grade books.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT * FROM schools ORDER BY name ASC";

    std::vector<SSQLS::schools> rows;
    query.storein(rows);

    out_gradeBooks.reserve(rows.size());
    for (const auto& row : rows) {
      AA::School s = schoolFromSSQLS(row);
      const auto ssqlsRow = s.toSSQLS();
      out_gradeBooks.emplace_back(std::string(ssqlsRow.name.c_str()),
                                  gradebookFromSchool(s));
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading all grade books: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::gradebook::loadGradeBookForSchool(
    const std::string& schoolName, AA::Gradebook& out_gradeBook) const {
  out_gradeBook = AA::Gradebook{};

  if (!m_conn.connected() || schoolName.empty()) {
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT * FROM schools WHERE name = " << mysqlpp::quote
          << schoolName << " LIMIT 1";

    std::vector<SSQLS::schools> rows;
    query.storein(rows);
    if (rows.empty()) {
      return false;
    }

    AA::School s = schoolFromSSQLS(rows.front());
    out_gradeBook = gradebookFromSchool(s);
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading grade book for school: ") +
                     e.what());
    return false;
  }
}

namespace {

std::string serializeGradeBook(const AA::Gradebook& gradeBook) {
  // Replace with your AA::Gradebook serialization method (e.g., toJsonString(),
  // toString(), or stream/JSON serialization) matching gradebookFromSchool.
  return gradeBook.getName();
}

}  // namespace

bool AA::Data::gradebook::updateGradeBookForSchool(
    const std::string& schoolName, const AA::Gradebook& gradeBook) {
  if (!m_conn.connected() || schoolName.empty()) {
    return false;
  }

  mysqlpp::Transaction trans(m_conn);

  try {
    const std::string serializedGradeBook = serializeGradeBook(gradeBook);

    mysqlpp::Query query = m_conn.query();
    query << "UPDATE schools SET gradebook = " << mysqlpp::quote
          << serializedGradeBook << " WHERE name = " << mysqlpp::quote
          << schoolName;

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed updating grade book for school: ") +
                       query.error());
      return false;
    }

    trans.commit();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception updating grade book for school: ") +
                     e.what());
    return false;
  }
}
