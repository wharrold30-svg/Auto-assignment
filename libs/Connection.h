#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <compare>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace Connection {

class NetworkError : public std::runtime_error {
 public:
  explicit NetworkError(const std::string& str)
      : std::runtime_error{"Network error: " + str} {}
};  // NetworkError

class Socket {
 private:
  friend class PortListener;

  int m_socket{-1};
  int safeRead(int fd, void* buf, std::size_t count);
  int safeWrite(int fd, const char* buf, std::size_t count);
  int readBuffer(int fd, char* buf, int count);
  int writeBuffer(int fd, const char* buf, int count);
  unsigned long readNetulong();
  void writeNetulong(const unsigned long value);

 protected:
  struct AdoptTag {};
  Socket(int fd, AdoptTag) noexcept : m_socket{fd} {}

  Socket(Socket&& other) noexcept : m_socket{other.releaseSocket()} {}

  Socket& operator=(Socket&& other) noexcept {
    if (this != &other) {
      if (m_socket >= 0) {
        ::close(m_socket);
      }
      m_socket = other.releaseSocket();
    }
    return *this;
  }

  int releaseSocket() noexcept {
    const int fd = m_socket;
    m_socket = -1;
    return fd;
  }

  void resetSocket(int fd) noexcept { m_socket = fd; }

  void closeOwnSocket() noexcept {
    const int fd = releaseSocket();
    if (fd >= 0) {
      ::close(fd);
    }
  }

 public:
  explicit Socket(const int socket) : m_socket{socket} {
    if (m_socket < 0) {
      throw NetworkError{"Socket creation error"};
    }
  }
  Socket() : Socket{::socket(AF_INET, SOCK_STREAM, 0)} {}

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  virtual ~Socket() { closeOwnSocket(); }

  std::optional<std::string> rx();
  bool sx(std::string_view str);
  int getSocket() const noexcept { return m_socket; }
};  // Socket

class Connection : public Socket {
 private:
  bool m_isOpen{true};
  std::string m_address;

 public:
  explicit Connection(int socket) : Socket{socket} {}
  Connection(std::string_view address, unsigned short port);

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&& other) noexcept
      : Socket{other.releaseSocket(), AdoptTag{}},
        m_isOpen{other.m_isOpen},
        m_address{std::move(other.m_address)} {
    other.setClosed();
  }

  Connection& operator=(Connection&& other) noexcept {
    if (this != &other) {
      closeSocket();
      const int movedFd = other.releaseSocket();
      resetSocket(movedFd);
      m_isOpen = other.m_isOpen && (movedFd >= 0);
      m_address = std::move(other.m_address);
      other.setClosed();
    }
    return *this;
  }

  ~Connection() override { closeSocket(); }

  std::strong_ordering operator<=>(const Connection& other) const noexcept {
    return getSocket() <=> other.getSocket();
  }
  bool operator==(const Connection& other) const noexcept {
    return getSocket() == other.getSocket();
  }
  std::strong_ordering operator<=>(int value) const noexcept {
    return getSocket() <=> value;
  }
  bool operator==(int value) const noexcept { return getSocket() == value; }
  explicit operator int() const noexcept { return getSocket(); }

  bool isOpen() const noexcept { return m_isOpen && (getSocket() >= 0); }
  explicit operator bool() const noexcept { return isOpen(); }

  void setClosed() noexcept { m_isOpen = false; }

  void closeSocket() noexcept {
    setClosed();
    closeOwnSocket();
  }

  const std::string& getAddr() const noexcept { return m_address; }
  void setAddr(std::string_view addr) { m_address = addr; }
};  // Connection

class PortListener {
 private:
  Socket m_socket;
  struct sockaddr_in m_address{};
  int m_opt = 1;

 public:
  explicit PortListener(unsigned short port);
  ~PortListener() = default;
  Connection startServer();
};  // PortListener

class ConnectContainer {
 private:
  std::string m_buffer;
  std::shared_ptr<Connection> m_conn;

 public:
  ConnectContainer(std::string_view buffer,
                   const std::shared_ptr<Connection>& conn)
      : m_buffer{buffer}, m_conn{conn} {}
  ConnectContainer() = default;

  const std::string& getBuffer() const noexcept { return m_buffer; }
  const std::shared_ptr<Connection>& getConn() const noexcept { return m_conn; }

  void setBuffer(std::string_view str) { m_buffer = str; }
  void setConn(const std::shared_ptr<Connection>& conn) { m_conn = conn; }

  void clear() noexcept {
    m_buffer.clear();
    m_conn.reset();
  }
};  // ConnectContainer

}  // namespace Connection

#endif  // CONNECTION_H
