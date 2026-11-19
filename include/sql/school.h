#ifndef SQL_SCHOOL_H
#define SQL_SCHOOL_H

#include <mysql++/mysql++.h>

#include <string>

#include "School.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class school : public DataBase<school, AA::School> {
 public:
  school(mysqlpp::Connection& conn) : DataBase<school, AA::School>{conn} {}
  school(const school&) = delete;
  school& operator=(const school&) = delete;
  school(const school&&) = delete;
  school& operator=(const school&&) = delete;
  ~school() = default;

  virtual std::string getTableName() const override { return "schools"; }

};  // school
}  // namespace Data
}  // namespace AA
#endif
