#ifndef GRADEBOOK_H
#define GRADEBOOK_H
#include <compare>
#include <string>

#include "nlohmann/json.hpp"
#include "sql/SSQLS.h"

namespace AA {
class Gradebook {
 private:
  std::string m_name;
  std::string m_url;
  // this is the template for the post string that must be sent to the grade
  // book to manipulate the grades
  std::string m_postTemp;

 public:
  Gradebook() = default;
  // getters
  const std::string& getName() const { return m_name; }
  const std::string& getURL() const { return m_url; }
  const std::string& getPostTemp() const { return m_postTemp; }

  // setters
  void setName(std::string_view str) { m_name = str; }
  void setURL(std::string_view str) { m_url = str; }
  void setPostTemp(std::string_view str) { m_postTemp = str; }

  std::strong_ordering operator<=>(const Gradebook& rhs) const {
    return m_name <=> rhs.m_name;
  }
  bool operator==(const Gradebook& rhs) const { return m_name == rhs.m_name; }

  // Converts domain Gradebook -> SSQLS struct (for database writes)
  SSQLS::gradebooks toSSQLS() const {
    SSQLS::gradebooks row;
    row.name = m_name;
    row.url = m_url;
    row.post_temp_script =
        m_postTemp;  // Or matching field name in your SSQLS definition
    return row;
  }

  static Gradebook fromSSQLS(const SSQLS::gradebooks& row) {
    Gradebook gb;
    gb.setName(row.name);
    gb.setURL(row.url);
    gb.setPostTemp(row.post_temp_script);
    return gb;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Gradebook, m_name, m_url, m_postTemp)
};
}  // namespace AA
#endif
