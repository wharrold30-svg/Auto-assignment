#ifndef SCHOOL_H
#define SCHOOL_H

#include <compare>
#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

namespace AA {
class School {
 private:
  std::string m_name;
  std::string m_address;
  std::string m_gradebook;
  // evenrually i'll have to have some for howbto parce the data taat im sending
  // to the gradebook or create my own gradebook

 public:
  School() = default;
  School(std::string_view name, std::string_view address,
         std::string_view gradebook)
      : m_name{name}, m_address{address}, m_gradebook{gradebook} {}

  // getters
  const std::string& getName() const { return m_name; }
  const std::string& getAddress() const { return m_address; }
  const std::string& getGradebook() const { return m_gradebook; }

  // setter
  void setName(std::string_view name) { m_name = name; }
  void setAddress(std::string_view address) { m_name = address; }
  void setGradebook(std::string_view gradebook) { m_name = gradebook; }
  std::strong_ordering operator<=>(const School& rhs) const {
    if (auto ret{m_name <=> rhs.m_name}; ret != 0) {
      return ret;
    }
    return m_address <=> rhs.m_address;
  }
  bool operator==(const School& rhs) const {
    return (m_name == rhs.m_name) && (m_address == rhs.m_address);
  }
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(School, m_name, m_address, m_gradebook)
};  // School
}  // namespace AA

#endif
