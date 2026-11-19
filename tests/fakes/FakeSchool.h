#ifndef TESTS_FAKES_FAKE_SCHOOL_H
#define TESTS_FAKES_FAKE_SCHOOL_H

#include <string>

#include "FakeDataBase.h"
#include "School.h"

namespace AA {
namespace Data {
namespace Fakes {

class school : public FakeDataBase<AA::School, std::string> {
 public:
  explicit school(KeyExtractor extractKey =
                      [](const AA::School& s) { return s.getName(); })
      : FakeDataBase<AA::School, std::string>{"schools",
                                              std::move(extractKey)} {}

  school(const school&) = delete;
  school& operator=(const school&) = delete;
  school(school&&) = delete;
  school& operator=(school&&) = delete;
  ~school() override = default;

  std::string getTableName() const override { return "schools"; }
};

}  // namespace Fakes
}  // namespace Data
}  // namespace AA

#endif
