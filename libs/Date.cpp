// Date implementation

#include "Date.h"

#include <ctime>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

void DT::Date::swap(DT::Date& otherDate) noexcept {
  std::swap(m_date.tm_mon, otherDate.m_date.tm_mon);
  std::swap(m_date.tm_mday, otherDate.m_date.tm_mday);
  std::swap(m_date.tm_year, otherDate.m_date.tm_year);
  std::swap(m_date.tm_hour, otherDate.m_date.tm_hour);
  std::swap(m_date.tm_min, otherDate.m_date.tm_min);
  std::swap(m_date.tm_sec, otherDate.m_date.tm_sec);
  std::swap(m_date.tm_isdst, otherDate.m_date.tm_isdst);
  std::swap(m_isLT, otherDate.m_isLT);
}
void DT::swap(DT::Date& one, DT::Date& other) noexcept { one.swap(other); }

bool DT::Date::validateDateTime(std::string_view str) const {
  std::string s(str);

  // 1. RFC 5322 with weekday (e.g., "Wed, 18 Nov 2026 14:30:00 +0000")
  static const std::regex r_rfc5322_with_day(
      R"(^[A-Za-z]{3},\s+\d{1,2}\s+[A-Za-z]{3}\s+\d{4}\s+\d{2}:\d{2}:\d{2}\s+[+-]\d{4}$)");

  // 2. RFC 5322 without weekday (e.g., "18 Nov 2026 14:30:00 +0000")
  static const std::regex r_rfc5322_no_day(
      R"(^\d{1,2}\s+[A-Za-z]{3}\s+\d{4}\s+\d{2}:\d{2}:\d{2}\s+[+-]\d{4}$)");

  // 3. SQL / Log format (e.g., "2026-11-18 14:30:00" with optional fractional
  // seconds)
  static const std::regex r_sql(
      R"(^\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}(\.\d+)?$)");

  // 4. ISO 8601 / RFC 3339 format (e.g., "2026-11-18T14:30:00Z" or with offset)
  static const std::regex r_iso8601(
      R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+-]\d{4})$)");

  return std::regex_match(s, r_rfc5322_with_day) ||
         std::regex_match(s, r_rfc5322_no_day) || std::regex_match(s, r_sql) ||
         std::regex_match(s, r_iso8601);
}

void DT::Date::convertToGmt() {
  std::time_t time = std::mktime(&m_date);
  m_date = *std::gmtime(&time);
  m_isLT = false;
}

void DT::Date::convertToLT() {
  std::time_t time = std::mktime(&m_date);
  m_date = *std::localtime(&time);
  m_isLT = true;
}

void DT::Date::setTM(std::string_view str) {
  if (str == " ") {
    std::time_t now = std::time(0);
    m_date = *std::gmtime(&now);
    m_isLT = false;
    return;
  }

  if (!validateDateTime(str)) {
    throw DateError("Incorret Date format");
  }

  std::string s(str);
  std::tm temp_tm{};
  bool parsed = false;

  // Try parsing RFC 5322 with weekday (e.g., "Wed, 18 Nov 2026 14:30:00 +0000")
  {
    std::istringstream ss(s);
    ss >> std::get_time(&temp_tm, "%a, %d %b %Y %H:%M:%S");
    if (!ss.fail()) parsed = true;
  }

  // Try parsing RFC 5322 without weekday (e.g., "18 Nov 2026 14:30:00 +0000")
  if (!parsed) {
    std::istringstream ss(s);
    ss >> std::get_time(&temp_tm, "%d %b %Y %H:%M:%S");
    if (!ss.fail()) parsed = true;
  }

  // Try parsing ISO 8601 / RFC 3339 (e.g., "2026-11-18T14:30:00Z")
  // Try parsing ISO 8601 / RFC 3339 (e.g., "2026-11-18T14:30:00Z")
  if (!parsed) {
    std::istringstream ss(s);
    ss >> std::get_time(&temp_tm, "%Y-%m-%dT%H:%M:%S");
    if (!ss.fail()) parsed = true;
  }

  // Try parsing SQL / Log format (e.g., "2026-11-18 14:30:00")
  if (!parsed) {
    std::istringstream ss(s);
    ss >> std::get_time(&temp_tm, "%Y-%m-%d %H:%M:%S");
    if (!ss.fail()) parsed = true;
  }

  if (!parsed) {
    throw DateError("Incorret Date format: Failed to parse fields");
  }

  m_date = temp_tm;

  normalize();
  convertToGmt();
}

DT::Date::Date() = default;

DT::Date::Date(const std::tm& date) : m_date(date), m_isLT(false) {
  convertToGmt();
}

DT::Date::Date(std::string_view date) { setTM(date); }

DT::Date::Date(const DT::Date& otherDate) : DT::Date(otherDate.m_date) {}

DT::Date& DT::Date::operator=(const DT::Date& rhs) {
  if (&rhs != this) {
    DT::Date copy(rhs);
    using std::swap;
    swap(*this, copy);
    normalize();
  }
  return *this;
}

// ============================================================================
// 1. RFC 5322 with Weekday (Email Header Standard)
// Example: "Wed, 18 Nov 2026 14:30:00 +0000"
// ============================================================================
std::string DT::Date::toStringWithDay() const {
  char buffer[100];
  // %a = abbreviated weekday, %d = day, %b = abbreviated month, %Y = year,
  // %H:%M:%S = time, %z = timezone offset (+HHMM)
  std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %z", &m_date);
  return std::string(buffer);
}

// ============================================================================
// 2. RFC 5322 without Weekday
// Example: "18 Nov 2026 14:30:00 +0000"
// ============================================================================
std::string DT::Date::toStringNoDay() const {
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%d %b %Y %H:%M:%S %z", &m_date);
  return std::string(buffer);
}

// ============================================================================
// 3. SQL / Log Format (Database Standard)
// Example: "2026-11-18 14:30:00"
// ============================================================================
std::string DT::Date::toStringSql() const {
  char buffer[100];
  // %Y-%m-%d = Date, %H:%M:%S = Time
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &m_date);
  return std::string(buffer);
}

// ============================================================================
// 4. ISO 8601 / RFC 3339 Format (API & JSON Standard)
// Example: "2026-11-18T14:30:00Z" (or using timezone offset)
// ============================================================================
std::string DT::Date::toStringJSON() const {
  char buffer[100];
  // %Y-%m-%dT%H:%M:%SZ separates date and time with 'T' and ends with Zulu 'Z'
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &m_date);
  return std::string(buffer);
}
bool DT::Date::empty() const {
  return m_date.tm_mon == 0 && m_date.tm_mday == 0 && m_date.tm_year == 0 &&
         m_date.tm_hour == 0 && m_date.tm_min == 0 && m_date.tm_sec == 0;
}

void DT::Date::clear() {
  m_date.tm_mon = 0;
  m_date.tm_mday = 0;
  m_date.tm_year = 0;
  m_date.tm_hour = 0;
  m_date.tm_min = 0;
  m_date.tm_sec = 0;
}

DT::Date::operator std::string() const { return toStringNoDay(); }

std::ostream& operator<<(std::ostream& os, const DT::Date& date) {
  os << date.toStringNoDay();
  return os;
}

std::time_t getCurrentTime() { return std::time(nullptr); }

DT::Date DT::getCurrentTimeGMT() {
  std::time_t now = getCurrentTime();
  std::tm ct = *std::gmtime(&now);
  return DT::Date{ct};
}

DT::Date DT::getCurrentTimeLT() {
  std::time_t now = getCurrentTime();
  std::tm ct = *std::localtime(&now);
  return DT::Date{ct};
}
