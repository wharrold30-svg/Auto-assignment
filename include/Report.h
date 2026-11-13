#ifndef REPORT_H
#define REPORT_H

#include <functional>
#include <iostream>
#include <map>
#include <span>

#include "Student.h"
#include "Test.h"

namespace AA {
using GradeMap = std::multimap<float, std::string, std::greater<float>>;
using DataIter = std::map<std::string, float>::const_iterator;

class Report {
 private:
  std::map<std::string, float> m_data;
  GradeMap five(std::function<void(std::span<DataIter>)> partial_sort_fn) const;
  float percentage(std::function<bool(const float)> func) const;
  static auto square(const auto num) { return num * num; }

 public:
  Report() = default;
  Report(const Test& test);

  GradeMap topFive() const;
  GradeMap bottomFive() const;
  float mean() const;
  float meadian() const;
  float var() const;
  float sd() const;
  float passPercentage() const;
  float failPercentage() const;
};  // Report

std::string buildTeacherReport(const Test& test);
std::string buildStundentReport(const std::shared_ptr<Student>& pStudent,
                                const Test& test);
std::size_t maxNameLength(const GradeMap& grades);
}  // namespace AA
std::ostream& operator<<(std::ostream& os, const AA::Report& report);
#endif  // REPORT_H
