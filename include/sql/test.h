#ifndef SQL_TEST_H
#define SQL_TEST_H

#include <mysql++/mysql++.h>

#include <string>
#include <vector>

#include "Date.h"
#include "OpenTest.h"
#include "Test.h"
#include "sql/DataBase.h"

namespace AA::Data {

class test : public DataBase<test, AA::Test> {
 public:
  explicit test(mysqlpp::Connection& conn) : DataBase<test, AA::Test>{conn} {}
  test(const test&) = delete;
  test& operator=(const test&) = delete;
  test(test&&) = delete;
  test& operator=(test&&) = delete;
  ~test() override = default;

  std::string getTableName() const override { return "tests"; }

  bool saveOpenTest(const AA::OpenTest& openTest);

  bool loadOpenTestsByDateRange(const DT::Date& startDate,
                                const DT::Date& endDate,
                                std::vector<AA::OpenTest>& out_tests) const;

  bool loadTestsPaginated(int limit, int offset,
                          std::vector<AA::Test>& out_tests) const;

  bool saveTestQuestions(const std::string& testId, const TestVec& questions);

  bool saveTestWithQuestions(const AA::Test& test);

  bool loadTestQuestions(const std::string& testId,
                         TestVec& out_questions) const;

  bool removeTestQuestions(const std::string& testId);

  bool loadTestWithQuestions(const std::string& testId,
                             AA::Test& out_test) const;
};  // class test

}  // namespace AA::Data

#endif  // SQL_TEST_H
