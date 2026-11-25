#ifndef AA_SERVER_CONTAINER_BRIDGE_H
#define AA_SERVER_CONTAINER_BRIDGE_H

#include <functional>
#include <string>

#include "Connection.h"
#include "RequestProcessor.h"
#include "sql/ConnectionPool.h"

namespace AA::Server {

class ContainerBridge {
 public:
  explicit ContainerBridge(AA::Data::ConnectionPool& pool) noexcept
      : m_processor{pool} {}

  void operator()(Connection::ConnectContainer& container) const {
    const std::string reply = m_processor(container.getBuffer());
    container.setBuffer(reply);
  }

  std::function<void(Connection::ConnectContainer&)> asCallback() const {
    return [processor = m_processor](Connection::ConnectContainer& container) {
      const std::string reply = processor(container.getBuffer());
      container.setBuffer(reply);
    };
  }

 private:
  RequestProcessor m_processor;
};

}  // namespace AA::Server

#endif  // AA_SERVER_CONTAINER_BRIDGE_H
