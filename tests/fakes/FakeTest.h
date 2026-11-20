#ifndef FAKE_TEST_H
#define FAKE_TEST_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "FakeStore.h"
#include "Test.h"

namespace AA::Fake {

class FakeTest {
 private:
  Store& m_store;

 public:
  explicit FakeTest(Store& store) : m_store(store) {}

  bool loadTestQuestions(std::string_view testId,
                         std::vector<AA::QandA>& out_questions) const {
    out_questions.clear();
    const auto it = m_store.testQuestions.find(std::string(testId));
    if (it != m_store.testQuestions.end()) {
      out_questions = it->second;
    }
    return true;
  }

  bool loadTestWithQuestions(std::string_view testId,
                             AA::Test& out_test) const {
    const auto tIt = m_store.tests.find(std::string(testId));
    if (tIt == m_store.tests.end()) {
      return false;
    }
    out_test = tIt->second;
    const auto qIt = m_store.testQuestions.find(std::string(testId));
    if (qIt != m_store.testQuestions.end()) {
      out_test.setTest(qIt->second);
    }
    return true;
  }

  bool removeTestQuestions(std::string_view testId) {
    const std::string tKey(testId);
    m_store.testQuestions.erase(tKey);
    const auto tIt = m_store.tests.find(tKey);
    if (tIt != m_store.tests.end()) {
      tIt->second.setTest({});
    }
    return true;
  }
};

}  // namespace AA::Fake

#endif  // FAKE_TEST_H
