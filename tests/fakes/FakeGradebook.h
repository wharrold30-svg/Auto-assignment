#ifndef TESTS_FAKES_FAKE_GRADEBOOK_H
#define TESTS_FAKES_FAKE_GRADEBOOK_H

#include <string>

#include "FakeDataBase.h"
#include "Gradebook.h"

namespace AA {
namespace Data {
namespace Fakes {

class gradebook : public FakeDataBase<AA::Gradebook, std::string> {
 public:
  explicit gradebook(KeyExtractor extractKey =
                         [](const AA::Gradebook& g) { return g.getName(); })
      : FakeDataBase<AA::Gradebook, std::string>{"gradebooks",
                                                 std::move(extractKey)} {}

  gradebook(const gradebook&) = delete;
  gradebook& operator=(const gradebook&) = delete;
  gradebook(gradebook&&) = delete;
  gradebook& operator=(gradebook&&) = delete;
  ~gradebook() override = default;

  std::string getTableName() const override { return "gradebooks"; }
};

}  // namespace Fakes
}  // namespace Data
}  // namespace AA

#endif
