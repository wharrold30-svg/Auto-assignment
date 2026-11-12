#ifndef I_CONNECTION_H
#define I_CONNECTION_H
#include <optional>
#include <string>

class IConnection {
 public:
  virtual ~IConnection() = default;
  virtual void sx(const std::string& msg) = 0;
  virtual std::optional<std::string> rx() = 0;  // Added receive function
};
#endif
