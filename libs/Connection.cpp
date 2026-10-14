// implementstion file for Connection::Connection

#include "Connection.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

inline int Connection::Socket::safeRead(int fd, void* buf, std::size_t count) {
  int retval;
  retval = read(fd, buf, count);
  if (retval == -1) {
    throw NetworkError{
        std::format("read {} bytes from fd {} FAILED", count, fd)};
  }
  return retval;
}

inline int Connection::Socket::safeWrite(int fd, const char* buf,
                                         std::size_t count) {
  int retval;
  retval = write(fd, buf, count);
  if (retval == -1) {
    throw NetworkError{
        std::format("write {} bytes to fd {} FAILED", count, fd)};
  }
  return retval;
}

inline int Connection::Socket::readBuffer(int fd, char* buf, int count) {
  if (count < 0) {
    throw NetworkError{"readBuffer count < 0"};
  }

  int status = 0;
  int result = 0;
  char* ptr = buf;
  while (status != count) {
    result = safeRead(fd, ptr + status, count - status);
    if (result < 0) {
      return result;
    }
    if (result == 0) {
      return status;
    }
    status += result;
  }

  return status;
}

inline int Connection::Socket::writeBuffer(int fd, const char* buf, int count) {
  if (count < 0) {
    throw NetworkError{"readBuffer count < 0"};
  }

  int status = 0;
  int result = 0;
  while (status != count) {
    result = safeWrite(fd, buf + status, count - status);
    if (result < 0) {
      return result;
    }
    if (result == 0) {
      return status;
    }
    status += result;
  }
  return status;
}

inline unsigned long Connection::Socket::readNetulong() {
  char buffer[sizeof(uint32_t)];
  int status = 0;

  status = readBuffer(m_socket, buffer, sizeof(uint32_t));
  if (status != sizeof(uint32_t)) {
    throw NetworkError{
        std::format("unexpected EOF during readNetulong, read {} bytes of {}",
                    status, sizeof(uint32_t))};
  }

  uint32_t* value;
  std::memcpy(value, buffer, sizeof(uint32_t));

  *value = ntohl(*value);

  return static_cast<unsigned long>(*value);
}

inline void Connection::Socket::writeNetulong(const unsigned long value) {
  char buffer[sizeof(uint32_t)];
  uint32_t temp;
  temp = htonl(value);
  std::memcpy(buffer, &temp, sizeof(temp));
  int status = 0;
  status = writeBuffer(m_socket, buffer, sizeof(temp));
  if (status != sizeof(temp)) {
    throw NetworkError{std::format("writeNetulong failed, wrote {} bytes of {}",
                                   status, sizeof(temp))};
  }
}

inline std::optional<std::string> Connection::Socket::rx() {
  unsigned long messageLength{readNetulong()};
  char buffer[messageLength];

  int n = readBuffer(m_socket, buffer, messageLength);
  if (n == 0) {
    return std::nullopt;
  }

  return std::string(buffer, n);
}

inline bool Connection::Socket::sx(std::string_view str) {
  writeNetulong(str.length());

  int n = writeBuffer(m_socket, str.data(), str.length());

  return static_cast<bool>(n);
}

Connection::Connection::Connection(std::string_view address,
                                   unsigned short port)
    : Socket() {
  sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);

  // convert IPv4 and IPv6 addresses from text to
  // to binary form
  if (inet_pton(AF_INET, address.data(), &servAddr.sin_addr) <= 0) {
    throw NetworkError{"Invalid address: Address not supported"};
  }

  // try to connect 10 times before throwing
  int iTry = 0;
  const int mRetry = 10;
  const int mRetryWait = 2;
  while (connect(getSocket(), reinterpret_cast<sockaddr*>(&servAddr),
                 sizeof(servAddr)) < 0) {
    ++iTry;
    if (iTry > mRetry) {
      throw NetworkError{"Failed to connext to server"};
    }
    sleep(mRetryWait);
  }
}

Connection::PortListener::PortListener(unsigned short port) {
  // fircefully attaching socket to the port
  if (setsockopt(m_socket.getSocket(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &m_opt, sizeof(m_opt)) < 0) {
    throw NetworkError{"setsocopt FAILD"};
  }
  m_address.sin_family = AF_INET;
  m_address.sin_addr.s_addr = htonl(INADDR_ANY);

  m_address.sin_port = htons(port);

  if (bind(m_socket.getSocket(), reinterpret_cast<sockaddr*>(&m_address),
           sizeof(m_address)) < 0) {
    throw NetworkError{"listen FAILD"};
  }
}

inline Connection::Connection Connection::PortListener::startServer() {
  return Connection{m_socket.getSocket()};
}
