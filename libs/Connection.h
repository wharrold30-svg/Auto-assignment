#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <compare>
#include <cstring>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Connection {
class NetworkError : public std::runtime_error {
 public:
  explicit NetworkError(const std::string& str)
      : std::runtime_error{"Network error: " + str} {}
};  // NetworkError

class Socket {
 private:
  int m_socket;
  inline int safeRead(int fd, void* buf, std::size_t count);
  inline int safeWrite(int fd, const char* buf, std::size_t count);
  inline int readBuffer(int fd, char* buf, int count);
  inline int writeBuffer(int fd, const char* buf, int count);
  inline unsigned long readNetulong();
  inline void writeNetulong(const unsigned long value);

 public:
  Socket(const int socket) : m_socket{socket} {
    if (m_socket < 0) {
      throw NetworkError{"Socket creation error"};
    }
  }
  Socket() : Socket{socket(AF_INET, SOCK_STREAM, 0)} {}

  inline std::optional<std::string> rx();
  inline bool sx(std::string_view str);
  inline int getSocket() const { return m_socket; }
};  // Socket

class Connection : public Socket {
 public:
  Connection(int socket) : Socket{socket} {}
  Connection(std::string_view address, unsigned short port);
  std::partial_ordering operator<=>(const Connection& other) const {
    return getSocket() <=> other.getSocket();
  }
  bool operator==(const Connection& other) const {
    return getSocket() == other.getSocket();
  }
  std::partial_ordering operator<=>(const int value) {
    return getSocket() <=> value;
  }
  bool operator==(const int value) const { return getSocket() == value; }
  operator int() const { return getSocket(); }

 private:
  bool m_isOpen{true};

 public:
  bool isOpen() const { return m_isOpen; }
  operator bool() const { return isOpen(); }
  void setClosed() { m_isOpen = false; }
  void closeSocket() {
    if (isOpen()) {
      if (close(getSocket()) < 0) {
        throw NetworkError{"close failed"};
      }
      setClosed();
    }
  }
  ~Connection() { closeSocket(); }

 private:
  std::string m_address;

 public:
  std::string getAddr() const { return m_address; }
  void setAddr(std::string_view addr) { m_address = addr; }
};  // Connection
class PortListener {
 private:
  Socket m_socket;
  struct sockaddr_in m_address;
  int m_opt = 1;

 public:
  explicit PortListener(unsigned short port);
  inline Connection startServer();
};  // PortListener
class ConnectContainer {
 private:
  std::string m_buffer;
  std::shared_ptr<Connection> m_conn;

 public:
  ConnectContainer(std::string_view buffer,
                   const std::shared_ptr<Connection>& conn)
      : m_buffer{buffer}, m_conn{conn} {}
  ConnectContainer() : ConnectContainer{" ", nullptr} {}

  // getters
  std::string getBuffer() const { return m_buffer; }
  std::shared_ptr<Connection> getConn() const { return m_conn; }

  // setters
  void setBuffer(std::string_view str) { m_buffer = str; }
  void setConn(const std::shared_ptr<Connection>& conn) { m_conn = conn; }

  // clear
  void clear() {
    m_buffer.clear();
    m_conn.reset();
  }
};  // ConnectContainer
}  // namespace Connection
#endif
