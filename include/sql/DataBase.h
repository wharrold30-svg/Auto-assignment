#ifndef SQL_DATABASE_H
#define SQL_DATABASE_H

#include <mysql++/mysql++.h>

#include <string>
#include <type_traits>

namespace AA::Data {

template <typename Bridge, typename Derived, typename IdType = std::string>
class DataBase {
 protected:
  mysqlpp::Connection& m_conn;

  Bridge& bridge() { return *static_cast<Bridge*>(this); }
  const Bridge& bridge() const { return *static_cast<const Bridge*>(this); }

 public:
  explicit DataBase(mysqlpp::Connection& conn) : m_conn{conn} {}

  virtual ~DataBase() = default;

  virtual std::string getTableName() const = 0;

  // 1. SAVE / INSERT using the AA:: domain object's toSSQLS()
  bool save(const Derived& model) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query.insert(model.toSSQLS());
      mysqlpp::SimpleResult res = query.execute();
      return static_cast<bool>(res);
    } catch (const mysqlpp::Exception&) {
      return false;
    }
  }

  // 2. UPDATE using MySQL++'s two-arg query.update(orig, updated)
  bool update(const Derived& model, const std::string& idColumn,
              IdType idValue) const {
    if (!m_conn.connected()) return false;
    try {
      Derived existing;
      if (!read(idColumn, idValue, existing)) {
        return false;
      }

      mysqlpp::Query query = m_conn.query();
      query.update(existing.toSSQLS(), model.toSSQLS());

      mysqlpp::SimpleResult res = query.execute();
      return static_cast<bool>(res);
    } catch (const mysqlpp::Exception&) {
      return false;
    }
  }

  // 3. READ
  bool read(const std::string& idColumn, IdType idValue,
            Derived& out_model) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query << "SELECT * FROM " << bridge().getTableName() << " WHERE "
            << idColumn << " = ";

      if constexpr (std::is_same_v<std::decay_t<IdType>, std::string> ||
                    std::is_convertible_v<IdType, const char*>) {
        query << mysqlpp::quote << idValue;
      } else {
        query << idValue;
      }

      mysqlpp::StoreQueryResult res = query.store();
      if (!res || res.num_rows() == 0) return false;

      out_model.fromSSQLS(res[0]);
      return true;
    } catch (const mysqlpp::Exception&) {
      return false;
    }
  }

  // 4. REMOVE
  bool remove(const std::string& idColumn, IdType idValue) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query << "DELETE FROM " << bridge().getTableName() << " WHERE "
            << idColumn << " = ";

      if constexpr (std::is_same_v<std::decay_t<IdType>, std::string> ||
                    std::is_convertible_v<IdType, const char*>) {
        query << mysqlpp::quote << idValue;
      } else {
        query << idValue;
      }

      mysqlpp::SimpleResult res = query.execute();
      return static_cast<bool>(res) && (res.rows() > 0);
    } catch (const mysqlpp::Exception&) {
      return false;
    }
  }
};

}  // namespace AA::Data

#endif  // SQL_DATABASE_H
