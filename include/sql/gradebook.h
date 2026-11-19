#ifndef SQL_GRADEBOOK_H
#define SQL_GRADEBOOK_H

#include <mysql++/mysql++.h>

#include <string>

#include "Gradebook.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class gradebook : public DataBase<gradebook, AA::Gradebook> {
 public:
  gradebook(mysqlpp::Connection& conn) : DataBase<gradebook, Gradebook>{conn} {}
  gradebook(const gradebook&) = delete;
  gradebook& operator=(const gradebook&) = delete;
  gradebook(const gradebook&&) = delete;
  gradebook& operator=(const gradebook&&) = delete;
  ~gradebook() = default;

  virtual std::string getTableName() const override { return "gradebooks"; }

};  // gradebook
}  // namespace Data
}  // namespace AA
#endif
