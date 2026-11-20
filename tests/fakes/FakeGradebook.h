#ifndef FAKE_GRADEBOOK_H
#define FAKE_GRADEBOOK_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Gradebook.h"
#include "School.h"
#include "FakeStore.h"

namespace AA::Fake {

class FakeGradebook {
 private:
  Store& m_store;

 public:
  explicit FakeGradebook(Store& store) : m_store(store) {}

  bool loadGradeBookForSchool(std::string_view schoolName,
                              AA::Gradebook& out_gradebook) const {
    const auto sIt = m_store.schools.find(std::string(schoolName));
    if (sIt == m_store.schools.end()) {
      return false;
    }
    const std::string& gbName = sIt->second.getGradebook();
    if (gbName.empty()) {
      return false;
    }
    const auto gIt = m_store.gradebooks.find(gbName);
    if (gIt == m_store.gradebooks.end()) {
      return false;
    }
    out_gradebook = gIt->second;
    return true;
  }

  bool updateGradeBookForSchool(std::string_view schoolName,
                                const AA::Gradebook& gradebook) {
    const auto sIt = m_store.schools.find(std::string(schoolName));
    if (sIt == m_store.schools.end()) {
      return false;
    }
    if (gradebook.getName().empty()) {
      return false;
    }
    m_store.gradebooks[gradebook.getName()] = gradebook;
    sIt->second.setGradebook(gradebook.getName());
    return true;
  }

  bool loadAllGradeBooks(
      std::vector<std::pair<std::string, AA::Gradebook>>& out_gradebooks)
      const {
    out_gradebooks.clear();
    for (const auto& [name, gb] : m_store.gradebooks) {
      out_gradebooks.emplace_back(name, gb);
    }
    return true;
  }
};

}  // namespace AA::Fake

#endif  // FAKE_GRADEBOOK_H

