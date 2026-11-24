#include "ServerFunctor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
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
    if (!conn || data.empty()) {
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

    auto pos = std::ranges::find(m_connectVec, conn);
    if (pos != m_connectVec.end()) {
      m_connectVec.erase(pos);
    }
  };

  while (true) {
    fd_set readSet;
    fd_set writeSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    const int listenFd = m_mainConn.getSocket();
    FD_SET(listenFd, &readSet);
    m_maxFd = listenFd;

    for (const auto& conn : m_connectVec) {
      const int fd = conn->getSocket();
      FD_SET(fd, &readSet);
      if (fd > m_maxFd) {
        m_maxFd = fd;
      }
      if (hasPendingWrite(conn)) {
        FD_SET(fd, &writeSet);
      }
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

      if (clientFd >= FD_SETSIZE) {
        close(clientFd);
        continue;
      }

      if (clientFd >= 0) {
        auto workerConn = std::make_shared<Connection::Connection>(clientFd);
        if (setsockopt(static_cast<int>(*workerConn), SOL_SOCKET, SO_KEEPALIVE,
                       reinterpret_cast<const char*>(&m_opt1),
                       sizeof(int)) < 0) {
          throw Connection::NetworkError{"SO_KEEPALIVE FAILED"};
        }
        if (setsockopt(workerConn->getSocket(), IPPROTO_TCP, TCP_KEEPIDLE,
                       reinterpret_cast<const char*>(&m_opt2),
                       sizeof(int)) < 0) {
          throw Connection::NetworkError{"TCP_KEEPIDLE FAILED"};
        }
        workerConn->setAddr(inet_ntoa(m_clientAddr.sin_addr));
        m_connectVec.push_back(std::move(workerConn));
      }
    }

    // 2. Read phase -> enqueue incoming messages
    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      ConnectPtr conn = *it;
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
      Connection::ConnectContainer temp{std::move(m_processQueue.front())};
      m_processQueue.pop();

      if (temp.getBuffer() == "exit") {
        dropConnection(temp.getConn());
      } else {
        m_process(temp);
        if (m_writeNeeded) {
          queueWrite(temp.getConn(), temp.getBuffer());
        }
      }
    }

    // 4. Write phase -> flush discrete queued messages when socket is writable
    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      ConnectPtr conn = *it;
      const int fd = conn->getSocket();

      if (FD_ISSET(fd, &writeSet)) {
        if (auto writeIt = pendingWrites.find(conn);
            writeIt != pendingWrites.end() && !writeIt->second.empty()) {
          bool sendFailed = false;
          while (!writeIt->second.empty()) {
            if (!conn->sx(writeIt->second.front())) {
              sendFailed = true;
              break;
            }
            writeIt->second.pop();
          }

          pendingWrites.erase(writeIt);

          if (sendFailed) {
            conn->closeSocket();
            it = m_connectVec.erase(it);
            continue;
          }
        }
      }
      ++it;
    }
  }
}
