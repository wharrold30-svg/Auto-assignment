#ifndef LOG_H
#define LOG_H
// this file jolds all of.the logs

#include <format>
#include <fstream>
#include <string>
#include <string_view>

#include "Date.h"
#include "Sync_stream.h"

namespace {
class Log {
 private:
  std::string m_file;
  void writeToFile(std::string_view msg) {
    std::ofstream out(m_file, std::ofstream::app);

    Sync::SyncStream s{out};

    DT::Date currentDate;
    currentDate.convertToLT();

    s(std::format("{} | {}", currentDate.str(), msg));
  }

 public:
  Log(std::string_view file) : m_file{file} {}
  virtual void operator()(std::string_view msg) { writeToFile(msg); }
};  // Log
}  // namespace

namespace Connection {
class ConnectiomErrorLog {
 public:
  ConnectiomErrorLog(std::string_view msg)
      : Log::Log{"log/error/connection -" + std::string{msg}} {}
};  // ConnectiomErrorLog
}  // namespace Connection

namespace Email {
class EmailErrorLog {
 public:
  EmailErrorLog(std::string_view msg)
      : Log::Log{"log/error/email -" + std::string{msg}} {}
};  // EmailErrorLog
}  // namespace Email

namespace Database {
class DatabaseErrorLog {
 public:
  DatabaseErrorLog(std::string_view msg)
      : Log::Log{"log/error/database -" + std::string{msg}} {}
};  // DatabaseErrorLog
}  // namespace Database

#endif
