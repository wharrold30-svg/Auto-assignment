#ifndef SERVER_FUNCTOR_H
#define SERVER_FUNCTOR_H
// this will be a functor thay will run whichever server
// we choose. you initialize it with a process function which wellbe used ti
// process each request

#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include "Connection.h"

namespace AA {
class ServerFunctor {
 private:
  using ConnectPtr = std::shared_ptr<Connection::Connection>;

  Connection::Connection m_mainConn;
  sockaddr_in m_clientAddr{};
  socklen_t m_addrLen{sizeof(m_clientAddr)};
  int m_opt1{1}, m_opt2{300};

  int m_maxFd{0};

  std::vector<ConnectPtr> m_connectVec;
  std::queue<Connection::ConnectContainer> m_processQueue;
  std::queue<Connection::ConnectContainer> m_writeQueue;
  std::optional<std::string> m_buffer;

  std::function<void(Connection::ConnectContainer&)> m_process;

  bool m_writeNeeded;

 public:
  ServerFunctor(unsigned short port, std::string_view serverName,
                std::function<void(Connection::ConnectContainer&)> process,
                bool writeNeeded = true);

  void operator()();

};  // ServerFunctor
}  // namespace AA
#endif
