// This class handles the date and time

#ifndef DATE_H  // header guard
#define DATE_H

#include <compare>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

namespace nlohmann {
template <>
struct adl_serializer<std::tm> {
  static void to_json(json& j, const std::tm& time) {
    j["month"] = time.tm_mon;
    j["day"] = time.tm_mday;
    j["year"] = time.tm_year;
    j["hour"] = time.tm_hour;
    j["min"] = time.tm_min;
    j["sec"] = time.tm_sec;
    j["isdst"] = time.tm_isdst;
  }

  static void from_json(const json& j, std::tm& time) {
    time.tm_mon = j["month"].get<int>();
    time.tm_mday = j["day"].get<int>();
    time.tm_year = j["year"].get<int>();
    time.tm_hour = j["hour"].get<int>();
    time.tm_min = j["min"].get<int>();
    time.tm_sec = j["sec"].get<int>();
    time.tm_isdst = j["isdst"].get<int>();
    std::mktime(&time);
  }
};  // adl_serializer
}  // namespace nlohmann

namespace DT {
class Date {
 private:
  std::tm m_date;
  bool m_isLT{false};
  bool validateDateTime(std::string_view str) const;

 public:
  Date();
  explicit Date(const std::tm& date);
  explicit Date(std::string_view date);

  Date(const Date& otherDate);
  Date& operator=(const Date& rhs);
  Date(Date&& otherDate) = default;
  Date& operator=(Date&& rhs) = default;

  ~Date() = default;

  // Member swap() used to implement ninmember swap()
  void swap(Date& otherDate) noexcept;
  friend void swap(Date& one, Date& other) noexcept;

  // getters
  inline std::tm getTM() const { return m_date; }
  std::string getStr() const;
  std::string getDBStr() const;
  explicit operator std::string() const;
  bool empty() const;
  inline bool isLT() const { return m_isLT; }

  // setters
  void setTM(std::string_view str = " ");
  inline void normalize() { std::mktime(&m_date); }
  void clear();
  // a pair of functions for converting m_date to either GMT or local time
  void convertToGmt();
  void convertToLT();

  // subtraction operator returss a double that isbthe
  // difference between the two different Date objectsin seconds.
  // iff the rhs is older than the lhs then a negative is returned
  double operator-(const Date& otherDate) const {
    std::tm rhs = otherDate.getTM();
    std::tm lhs = getTM();
    return std::difftime(std::mktime(&rhs), std::mktime(&lhs));
  }
  // spaceship operator and equal-to operator
  // newer dates are less than older dates; therefore,
  //(lhs < rhs) will be true if lhs is newer than rhs
  std::partial_ordering operator<=>(const Date& rhs) const {
    return (*this - rhs) <=> 0.0;
  }
  bool operator==(const Date& rhs) const { return (*this - rhs) == 0.0; }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Date, m_date, m_isLT)

};  // Date

void swap(Date& one, Date& other) noexcept;

class DateError : public std::runtime_error {
 public:
  explicit DateError(std::string_view msg)
      : std::runtime_error(std::format("Date error: {}", msg)) {}
};  // DateError

Date getCurrentTimeGMT();
Date getCurrentTimeLT();
}  // namespace DT

std::ostream& operator<<(std::ostream& os, const DT::Date& date);

std::time_t getCurrentTime();
#endif  // header guard
