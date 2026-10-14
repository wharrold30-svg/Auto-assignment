#ifndef SYNC_STREAM_H
#define SYNC_STREAM_H

#include <iostream>
#include <mutex>
#include <sstream>
#include <string_view>

namespace Sync {

class Transaction {
 private:
  std::ostringstream m_buffer;

  friend std::ostream& operator<<(std::ostream& os, Transaction const& t);

 public:
  // Default to empty string instead of " " to avoid unwanted leading spaces
  Transaction(std::string_view str = "") { m_buffer << str; }

  template <class T>
  Transaction& operator<<(T const& t) {
    m_buffer << t;
    return *this;
  }

  std::string str() const { return m_buffer.str(); }
};

inline std::ostream& operator<<(std::ostream& os, Sync::Transaction const& t) {
  return os << t.m_buffer.str();
}

class SyncStream {
 private:
  std::ostream& m_out;
  std::mutex m_mutex;

 public:
  explicit SyncStream(std::ostream& sink) : m_out(sink) {}

  // Synchronize writes to the output stream under m_mutex protection
  template <typename T>
  SyncStream& operator<<(T const& input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_out << input;
    return *this;
  }
};

}  // namespace Sync

#endif  // SYNC_STREAM_H
