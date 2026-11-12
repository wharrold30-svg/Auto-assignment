// implementstion file for Connection::Connection

#include "Connection.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <format>

int Connection::Socket::safeRead(int fd, void* buf, std::size_t count) {
  int retval;
  retval = read(fd, buf, count);
  if (retval == -1) {
    throw NetworkError{
        std::format("read {} bytes from fd {} FAILED", count, fd)};
  }
  return retval;
}

int Connection::Socket::safeWrite(int fd, const char* buf, std::size_t count) {
  int retval;
  retval = write(fd, buf, count);
  if (retval == -1) {
    throw NetworkError{
        std::format("write {} bytes to fd {} FAILED", count, fd)};
  }
  return retval;
}

int Connection::Socket::readBuffer(int fd, char* buf, int count) {
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

int Connection::Socket::writeBuffer(int fd, const char* buf, int count) {
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

unsigned long Connection::Socket::readNetulong() {
  char buffer[sizeof(uint32_t)];
  int status = 0;

  status = readBuffer(m_socket, buffer, sizeof(uint32_t));
  if (status != sizeof(uint32_t)) {
    throw NetworkError{
        std::format("unexpected EOF during readNetulong, read {} bytes of {}",
                    status, sizeof(uint32_t))};
  }

  uint32_t value;
  std::memcpy(&value, buffer, sizeof(value));

  value = ntohl(value);

  return static_cast<unsigned long>(value);
}

void Connection::Socket::writeNetulong(const unsigned long value) {
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

std::optional<std::string> Connection::Socket::rx() {
  unsigned long messageLength{readNetulong()};

  // 1. Guard against empty reads or maliciously huge lengths
  constexpr unsigned long MAX_MESSAGE_SIZE = 10 * 1024 * 1024;  // e.g., 10 MB
  if (messageLength == 0 || messageLength > MAX_MESSAGE_SIZE) {
    return std::nullopt;
  }

  // 2. Allocate directly inside a std::string (avoids VLAs and extra copies)
  std::string buffer(messageLength, '\0');

  int n = readBuffer(m_socket, buffer.data(), messageLength);

  // 3. Check for disconnection or partial reads/errors
  if (n <= 0 || static_cast<unsigned long>(n) < messageLength) {
    return std::nullopt;
  }

  buffer.resize(n);
  return buffer;
}

bool Connection::Socket::sx(std::string_view str) {
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

Connection::Connection Connection::PortListener::startServer() {
  return Connection{m_socket.getSocket()};
}
