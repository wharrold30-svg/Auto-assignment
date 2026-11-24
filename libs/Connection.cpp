// implementstion file for Connection::Connection

#include "Connection.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <format>
#include <limits>

int Connection::Socket::safeRead(int fd, void* buf, std::size_t count) {
  while (true) {
    ssize_t retval = recv(fd, buf, count, 0);
    if (retval < 0) {
      if (errno == EINTR) continue;       // Interrupted by signal, retry
      if (errno == ECONNRESET) return 0;  // Treat connection reset as clean EOF
      throw NetworkError{std::format("recv {} bytes from fd {} FAILED: {}",
                                     count, fd, std::strerror(errno))};
    }
    return static_cast<int>(retval);
  }
}

int Connection::Socket::safeWrite(int fd, const char* buf, std::size_t count) {
  while (true) {
    // MSG_NOSIGNAL prevents SIGPIPE from crashing the server if peer
    // disconnected
    ssize_t retval = send(fd, buf, count, MSG_NOSIGNAL);
    if (retval < 0) {
      if (errno == EINTR) continue;  // Interrupted by signal, retry
      if (errno == EPIPE || errno == ECONNRESET)
        return 0;  // Peer closed socket
      throw NetworkError{std::format("send {} bytes to fd {} FAILED: {}", count,
                                     fd, std::strerror(errno))};
    }
    return static_cast<int>(retval);
  }
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
  // 1. Read the 4-byte length header
  char header[sizeof(uint32_t)];
  int headerBytes = readBuffer(m_socket, header, sizeof(uint32_t));

  // Clean EOF (client disconnected before sending next message)
  if (headerBytes == 0) {
    return std::nullopt;
  }
  // Partial header (client disconnected mid-header)
  if (headerBytes < static_cast<int>(sizeof(uint32_t))) {
    return std::nullopt;
  }

  uint32_t netLen = 0;
  std::memcpy(&netLen, header, sizeof(netLen));
  uint32_t messageLength = ntohl(netLen);

  // 2. Guard against zero-length or oversized payloads (max 10 MB)
  constexpr uint32_t kMaxMessageSize = 10 * 1024 * 1024;
  if (messageLength == 0 || messageLength > kMaxMessageSize) {
    return std::nullopt;
  }

  // 3. Read the payload directly into std::string
  std::string buffer(messageLength, '\0');
  int bodyBytes =
      readBuffer(m_socket, buffer.data(), static_cast<int>(messageLength));

  if (bodyBytes < static_cast<int>(messageLength)) {
    return std::nullopt;  // Client disconnected mid-payload
  }
  return buffer;
}

bool Connection::Socket::sx(std::string_view str) {
  if (str.size() > std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  // Pack 4-byte length header + payload into one contiguous buffer (1 syscall,
  // no Nagle delay)
  uint32_t netLen = htonl(static_cast<uint32_t>(str.size()));
  std::string packet;
  packet.resize(sizeof(netLen) + str.size());
  std::memcpy(packet.data(), &netLen, sizeof(netLen));
  std::memcpy(packet.data() + sizeof(netLen), str.data(), str.size());

  int totalSize = static_cast<int>(packet.size());
  int sent = writeBuffer(m_socket, packet.data(), totalSize);

  return sent == totalSize;
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
  if (setsockopt(m_socket.getSocket(), SOL_SOCKET, SO_REUSEADDR, &m_opt,
                 sizeof(m_opt)) < 0) {
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
  if (listen(m_socket.getSocket(), SOMAXCONN) < 0) {
    throw NetworkError{"listen FAILED"};
  }
  return Connection{m_socket.getSocket()};
}
