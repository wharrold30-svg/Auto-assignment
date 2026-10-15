#ifndef LOG_H
#define LOG_H

// this file holds all of the log classes
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Date.h"
#include "Sync_stream.h"

namespace Log {
class Log {
 private:
  std::string m_file;
  void writeToFile(std::string_view msg) {
    auto parentPath = std::filesystem::path(m_file).parent_path();
    if (!parentPath.empty()) {
      std::filesystem::create_directories(parentPath);
    }
    std::ofstream out(m_file, std::ofstream::app);
    if (!out) {
      throw std::runtime_error{std::format("Could not open {}", m_file)};
    }

    Sync::SyncStream s{out};

    DT::Date currentDate;
    currentDate.convertToLT();

    s << std::format("{} | {}\n", currentDate.getStr(), msg);
  }

 public:
  Log(std::string_view file) : m_file{file} {}
  virtual ~Log() = default;
  virtual void operator()(std::string_view msg) { writeToFile(msg); }
};  // Log
}  // namespace Log

namespace Connection {
class ConnectionErrorLog : public Log::Log {
 public:
  ConnectionErrorLog(std::string_view msg)
      : Log::Log{"log/error/connection.log"} {
    (*this)(msg);
  }
};  // ConnectionErrorLog
}  // namespace Connection

namespace Email {
class EmailErrorLog : public Log::Log {
 public:
  EmailErrorLog(std::string_view msg) : Log::Log{"log/error/email.log"} {
    (*this)(msg);
  }
};  // EmailErrorLog
}  // namespace Email

namespace Database {
class DatabaseErrorLog : public Log::Log {
 public:
  DatabaseErrorLog(std::string_view msg) : Log::Log{"log/error/database.log"} {
    (*this)(msg);
  }
};  // DatabaseErrorLog
}  // namespace Database
#endif
