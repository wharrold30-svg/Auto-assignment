#include "ServerFunctor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <format>
#include <iostream>

AA::ServerFunctor::ServerFunctor(
    unsigned short port, std::string_view serverName,
    std::function<void(Connection::ConnectContainer&)> process,
    bool writeNeeded)
    : m_mainConn{Connection::PortListener{port}.startServer()},
      m_process{process},
      m_writeNeeded{writeNeeded} {
  std::cout << std::format(
      "The \"{}\" server is active, listening on port {} /tPID: {}\n",
      serverName, port, getpid());
  FD_ZERO(&m_origFdset);
  FD_SET(static_cast<int>(m_mainConn), &m_origFdset);
  m_maxFd = m_mainConn.getSocket();
}

void AA::ServerFunctor::operator()() {
  auto updateMaxFd = [this]() {
    m_maxFd = m_mainConn.getSocket();
    for (const auto& conn : m_connectVec) {
      if (conn->getSocket() > m_maxFd) {
        m_maxFd = conn->getSocket();
      }
    }
  };

  while (true) {
    // restore watch set as appropriate
    bcopy(&m_origFdset, &m_fdset, sizeof(m_origFdset));

    select(m_maxFd + 1, &m_fdset, static_cast<fd_set*>(0),
           static_cast<fd_set*>(0), static_cast<timeval*>(0));

    if (FD_ISSET(m_mainConn.getSocket(), &m_fdset)) {
      // new connection
      // possibly log all connections in the future
      m_addrLen = sizeof(m_clientAddr);  // Reset size before every accept()

      int clientFd =
          accept(m_mainConn.getSocket(),
                 reinterpret_cast<sockaddr*>(&m_clientAddr), &m_addrLen);

      if (clientFd >= FD_SETSIZE) {
        close(clientFd);
        continue;  // Prevent fd_set buffer overflow
      }

      if (clientFd >= 0) {
        m_workerConn = std::make_shared<Connection::Connection>(clientFd);
        if (setsockopt(static_cast<int>(*m_workerConn), SOL_SOCKET,
                       SO_KEEPALIVE, reinterpret_cast<const char*>(&m_opt1),
                       sizeof(int)) < 0) {
          if (clientFd > m_maxFd) {
            m_maxFd = clientFd;
          }
          throw Connection::NetworkError{"SO_KEEPALIVE FAILED"};
        }
        if (setsockopt(m_workerConn->getSocket(), IPPROTO_TCP, TCP_KEEPIDLE,
                       reinterpret_cast<const char*>(&m_opt2),
                       sizeof(int)) < 0) {
          throw Connection::NetworkError{"TCP_KEEPIDLE FAILED"};
        }
        if (clientFd > m_maxFd) {
          m_maxFd = clientFd;
        }
        m_workerConn->setAddr(inet_ntoa(m_clientAddr.sin_addr));
        FD_SET(m_workerConn->getSocket(), &m_origFdset);
        m_connectVec.push_back(m_workerConn);
      }
    }

    // add to process queue
    for (auto it = m_connectVec.begin(); it != m_connectVec.end();) {
      int fd = (*it)->getSocket();
      if (FD_ISSET(fd, &m_fdset)) {
        m_buffer = (*it)->rx();
        if (!m_buffer || m_buffer.value() == "exit") {
          int closedFd = (*it)->getSocket();
          FD_CLR(fd, &m_origFdset);
          (*it)->closeSocket();
          it = m_connectVec.erase(it);
          if (closedFd == m_maxFd) {
            updateMaxFd();
          }
          continue;
        }
        m_processQueue.emplace(std::move(*m_buffer), *it);
      }
      ++it;
    }

    // process items in thr process queue
    // add towwrite queue if needed
    while (!m_processQueue.empty()) {
      Connection::ConnectContainer temp{std::move(m_processQueue.front())};
      if (temp.getBuffer() == "exit") {
        int closedFd = temp.getConn()->getSocket();
        FD_CLR(temp.getConn()->getSocket(), &m_origFdset);
        temp.getConn()->closeSocket();
        m_connectVec.erase(std::ranges::find(m_connectVec, temp.getConn()));
        if (closedFd == m_maxFd) {
          updateMaxFd();
        }
      } else {
        m_process(temp);
        if (m_writeNeeded) {
          m_writeQueue.push(std::move(temp));
        }
      }
      m_processQueue.pop();
    }

    // process the write queue
    while (!m_writeQueue.empty()) {
      Connection::ConnectContainer temp{std::move(m_writeQueue.front())};
      if (!temp.getConn()->sx(temp.getBuffer())) {
        int closedFd = temp.getConn()->getSocket();
        FD_CLR(temp.getConn()->getSocket(), &m_origFdset);
        temp.getConn()->closeSocket();
        m_connectVec.erase(std::ranges::find(m_connectVec, temp.getConn()));
        if (closedFd == m_maxFd) {
          updateMaxFd();
        }
      }
      m_writeQueue.pop();
    }
  }
}
