#ifndef TESTS_FAKES_FAKE_DATABASE_H
#define TESTS_FAKES_FAKE_DATABASE_H

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace AA {
namespace Data {
namespace Fakes {

template <typename Domain, typename IdType = std::string>
class FakeDataBase {
 public:
  using KeyExtractor = std::function<IdType(const Domain&)>;

 private:
  std::string m_tableName;
  KeyExtractor m_extractKey;
  std::unordered_map<IdType, Domain> m_rows;

 public:
  explicit FakeDataBase(std::string tableName, KeyExtractor extractKey = {})
      : m_tableName{std::move(tableName)},
        m_extractKey{std::move(extractKey)} {}

  virtual ~FakeDataBase() = default;

  virtual std::string getTableName() const { return m_tableName; }

  void setKeyExtractor(KeyExtractor extractKey) {
    m_extractKey = std::move(extractKey);
  }

  bool save(const Domain& model) {
    if (!m_extractKey) return false;
    const IdType key = m_extractKey(model);
    if (m_rows.find(key) != m_rows.end()) {
      return false;  // Simulate unique primary key constraint
    }
    m_rows.emplace(key, model);
    return true;
  }

  bool update(const Domain& model, const std::string& /*idColumn*/,
              IdType idValue) {
    auto it = m_rows.find(idValue);
    if (it == m_rows.end()) {
      return false;
    }
    it->second = model;
    return true;
  }

  bool read(const std::string& /*idColumn*/, IdType idValue,
            Domain& out_model) const {
    auto it = m_rows.find(idValue);
    if (it == m_rows.end()) {
      return false;
    }
    out_model = it->second;
    return true;
  }

  bool remove(const std::string& /*idColumn*/, IdType idValue) {
    return m_rows.erase(idValue) > 0;
  }

  void clear() { m_rows.clear(); }
  std::size_t size() const { return m_rows.size(); }
};

}  // namespace Fakes
}  // namespace Data
}  // namespace AA

#endif
