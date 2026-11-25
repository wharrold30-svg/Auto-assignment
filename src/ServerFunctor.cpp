#include "ServerFunctor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <format>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include "Log.h"

AA::ServerFunctor::ServerFunctor(
    unsigned short port, std::string_view serverName,
    std::function<void(Connection::ConnectContainer&)> process,
    bool writeNeeded)
    : m_mainConn{Connection::PortListener{port}.startServer()},
      m_maxFd{m_mainConn.getSocket()},
      m_process{std::move(process)},
      m_writeNeeded{writeNeeded} {
  if (m_maxFd < 0 || m_maxFd >= FD_SETSIZE) {
    throw Connection::NetworkError{
        "Listening socket descriptor out of FD_SETSIZE range"};
  }

  std::cout << std::format(
      "The \"{}\" server is active, listening on port {} \tPID: {}\n",
      serverName, port, getpid());
}

void AA::ServerFunctor::operator()() {
  std::unordered_map<ConnectPtr, std::queue<std::string>> pendingWrites;

  auto hasPendingWrite = [&pendingWrites](const ConnectPtr& conn) -> bool {
    auto it = pendingWrites.find(conn);
    return it != pendingWrites.end() && !it->second.empty();
  };

  auto queueWrite = [&pendingWrites](const ConnectPtr& conn, std::string data) {
    if (!conn || !conn->isOpen() || data.empty()) {
      return;
    }
    pendingWrites[conn].push(std::move(data));
  };

  auto dropConnection = [this, &pendingWrites](const ConnectPtr& conn) {
    if (!conn) {
      return;
    }
    conn->closeSocket();
    pendingWrites.erase(conn);

    std::erase(m_connectVec, conn);
  };

  while (true) {
    fd_set readSet;
    fd_set writeSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    const int listenFd = m_mainConn.getSocket();
    if (listenFd < 0 || listenFd >= FD_SETSIZE) {
      throw Connection::NetworkError{
          "Listening socket descriptor out of FD_SETSIZE range"};
    }

    FD_SET(listenFd, &readSet);
    m_maxFd = listenFd;

    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      const ConnectPtr& conn = *it;
      if (!conn || !conn->isOpen()) {
        pendingWrites.erase(conn);
        it = m_connectVec.erase(it);
        continue;
      }

      const int fd = conn->getSocket();
      if (fd < 0 || fd >= FD_SETSIZE) {
        AA::SaveErrorLog(std::format(
            "Client fd {} out of FD_SETSIZE range; closing connection", fd));
        conn->closeSocket();
        pendingWrites.erase(conn);
        it = m_connectVec.erase(it);
        continue;
      }

      FD_SET(fd, &readSet);
      if (fd > m_maxFd) {
        m_maxFd = fd;
      }
      if (hasPendingWrite(conn)) {
        FD_SET(fd, &writeSet);
      }
      ++it;
    }

    const int ready =
        select(m_maxFd + 1, &readSet, &writeSet, nullptr, nullptr);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      AA::SaveErrorLog(std::string("select() failed: ") + std::strerror(errno));
      continue;
    }

    // 1. Accept new connections
    if (FD_ISSET(listenFd, &readSet)) {
      m_addrLen = sizeof(m_clientAddr);
      const int clientFd = accept(
          listenFd, reinterpret_cast<sockaddr*>(&m_clientAddr), &m_addrLen);

      if (clientFd >= 0) {
        if (clientFd >= FD_SETSIZE) {
          AA::SaveErrorLog(
              std::format("Accepted fd {} exceeds FD_SETSIZE ({}); closing",
                          clientFd, FD_SETSIZE));
          ::close(clientFd);
        } else {
          auto workerConn = std::make_shared<Connection::Connection>(clientFd);
          bool sockOptOk = true;

          if (setsockopt(workerConn->getSocket(), SOL_SOCKET, SO_KEEPALIVE,
                         reinterpret_cast<const char*>(&m_opt1),
                         sizeof(int)) < 0) {
            AA::SaveErrorLog(std::string("SO_KEEPALIVE failed: ") +
                             std::strerror(errno));
            sockOptOk = false;
          }

          if (sockOptOk &&
              setsockopt(workerConn->getSocket(), IPPROTO_TCP, TCP_KEEPIDLE,
                         reinterpret_cast<const char*>(&m_opt2),
                         sizeof(int)) < 0) {
            AA::SaveErrorLog(std::string("TCP_KEEPIDLE failed: ") +
                             std::strerror(errno));
            sockOptOk = false;
          }

          if (!sockOptOk) {
            workerConn->closeSocket();
          } else {
            workerConn->setAddr(inet_ntoa(m_clientAddr.sin_addr));
            m_connectVec.push_back(std::move(workerConn));
          }
        }
      } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        AA::SaveErrorLog(std::string("accept() failed: ") +
                         std::strerror(errno));
      }
    }

    // 2. Read phase -> enqueue incoming messages
    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      ConnectPtr conn = *it;
      if (!conn || !conn->isOpen()) {
        pendingWrites.erase(conn);
        it = m_connectVec.erase(it);
        continue;
      }

      const int fd = conn->getSocket();
      if (FD_ISSET(fd, &readSet)) {
        m_buffer = conn->rx();
        if (!m_buffer || m_buffer.value() == "exit") {
          conn->closeSocket();
          pendingWrites.erase(conn);
          it = m_connectVec.erase(it);
          continue;
        }
        m_processQueue.emplace(std::move(*m_buffer), conn);
      }
      ++it;
    }

    // 3. Process queue -> run request processor and queue outbound frames
    while (!m_processQueue.empty()) {
      Connection::ConnectContainer& current = m_processQueue.front();
      ConnectPtr conn = current.getConn();

      if (conn && conn->isOpen()) {
        m_process(current);
        if (m_writeNeeded) {
          queueWrite(conn, current.getBuffer());
        }
      }

      m_processQueue.pop();
    }

    // 4. Write phase -> send one queued message per ready socket per tick
    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      const ConnectPtr& conn = *it;
      const int fd = conn ? conn->getSocket() : -1;
      if (!conn || !conn->isOpen() || fd < 0 || fd >= FD_SETSIZE) {
        if (fd >= FD_SETSIZE) {
          AA::SaveErrorLog(std::format(
              "Client fd {} out of FD_SETSIZE range; closing connection", fd));
        }
        if (conn) {
          conn->closeSocket();
        }
        pendingWrites.erase(conn);
        it = m_connectVec.erase(it);
        continue;
      }

      if (FD_ISSET(fd, &writeSet) && hasPendingWrite(conn)) {
        auto& q = pendingWrites[conn];
        if (!conn->sx(q.front())) {
          conn->closeSocket();
          pendingWrites.erase(conn);
          it = m_connectVec.erase(it);
          continue;
        }
        q.pop();
        if (q.empty()) {
          pendingWrites.erase(conn);
        }
      }
      ++it;
    }
  }
}
