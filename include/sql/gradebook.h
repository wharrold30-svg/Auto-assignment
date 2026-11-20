#ifndef SQL_GRADEBOOK_H
#define SQL_GRADEBOOK_H

#include <mysql++/mysql++.h>

#include <string>
#include <utility>
#include <vector>

#include "Gradebook.h"
#include "sql/DataBase.h"

namespace AA {
namespace Data {
class gradebook : public DataBase<gradebook, AA::Gradebook> {
 public:
  gradebook(mysqlpp::Connection& conn)
      : DataBase<gradebook, AA::Gradebook>{conn} {}
  gradebook(const gradebook&) = delete;
  gradebook& operator=(const gradebook&) = delete;
  gradebook(const gradebook&&) = delete;
  gradebook& operator=(const gradebook&&) = delete;
  ~gradebook() = default;

  virtual std::string getTableName() const override { return "gradebooks"; }

  bool loadAllGradeBooks(
      std::vector<std::pair<std::string, AA::Gradebook>>& out_gradeBooks) const;

  bool loadGradeBookForSchool(const std::string& schoolName,
                              AA::Gradebook& out_gradeBook) const;

  bool updateGradeBookForSchool(const std::string& schoolName,
                                const AA::Gradebook& gradeBook);

};  // gradebook
}  // namespace Data
}  // namespace AA
#endif
