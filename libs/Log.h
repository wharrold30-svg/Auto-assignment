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
// The core logging class that handles writing to disk.
// We keep this as a regular class because it holds state (the file path).
class Log {
 private:
  std::string m_file;
  void writeToFile(std::string_view msg) const {
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
  explicit Log(std::string_view file) : m_file{file} {}

  // Overload operator() so instances of this class can be called like functions
  void operator()(std::string_view msg) const { writeToFile(msg); }
};  // Log
}  // namespace Log

namespace Connection {
struct ConnectionErrorLogFunctor {
  void operator()(std::string_view msg) const {
    Log::Log{"log/error/connection.log"}(msg);
  }
};
// Global function object instance
inline constexpr ConnectionErrorLogFunctor ConnectionErrorLog{};
}  // namespace Connection

namespace Email {
struct EmailErrorLogFunctor {
  void operator()(std::string_view msg) const {
    Log::Log{"log/error/email.log"}(msg);
  }
};
// Global function object instance
inline constexpr EmailErrorLogFunctor EmailErrorLog{};
}  // namespace Email

namespace Database {
struct DatabaseErrorLogFunctor {
  void operator()(std::string_view msg) const {
    Log::Log{"log/error/database.log"}(msg);
  }
};
// Global function object instance
inline constexpr DatabaseErrorLogFunctor DatabaseErrorLog{};
}  // namespace Database

namespace AA {
struct SaveErrorLogFunctor {
  void operator()(std::string_view msg) const {
    Log::Log{"log/error/database.log"}(msg);
  }
};

struct LoadErrorLogFunctor {
  void operator()(std::string_view msg) const {
    Log::Log{"log/error/database.log"}(msg);
  }
};

// Global function object instances
inline constexpr SaveErrorLogFunctor SaveErrorLog{};
inline constexpr LoadErrorLogFunctor LoadErrorLog{};
}  // namespace AA

#endif
