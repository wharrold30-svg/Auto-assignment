#ifndef REAL_CONNECTION_H
#define REAL_CONNECTION_H
#include <optional>

#include "Connection.h"
#include "IConnection.h"

class RealConnection : public IConnection {
  Connection::Connection m_conn;

 public:
  RealConnection(const std::string& host, int port) : m_conn(host, port) {}

  void sx(const std::string& msg) override { m_conn.sx(msg); }

  std::optional<std::string> rx() override {
    return m_conn.rx();  // Assuming your Connection class has a corresponding
                         // rx() method
  }
};
#endif
