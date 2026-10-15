#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace Signal {

class SignalException : public std::runtime_error {
 public:
  explicit SignalException(std::string_view message)
      : std::runtime_error(message.data()) {}
};

class SignalHandler {
 private:
  int m_signum;
  std::function<void(int)> m_handler;
  struct sigaction m_act;

  // Static registry mapping signal numbers to modern functional handlers
  static inline std::unordered_map<int, std::function<void(int)>>
      s_signal_registry;

  // The single, centralized raw C-style function pointer that POSIX expects
  static void global_signal_router(int signum, siginfo_t* info, void* context) {
    // Find and execute the registered C++ functional handler
    if (auto it = s_signal_registry.find(signum);
        it != s_signal_registry.end()) {
      if (it->second) {
        it->second(signum);
      }
    }
  }

 public:
  // Constructor now gracefully accepts modern std::function or lambdas
  SignalHandler(int signum, std::function<void(int)> handler)
      : m_signum{signum}, m_handler{std::move(handler)}, m_act{} {
    // Register this instance's handler in our static routing map
    s_signal_registry[m_signum] = m_handler;

    m_act.sa_flags = SA_SIGINFO;
    m_act.sa_sigaction = &SignalHandler::global_signal_router;
  }

  void setUpSignalHandler() {
    if (sigaction(m_signum, &m_act, nullptr) == -1) {
      throw SignalException{"**Error setting up signal handler**"};
    }
  }
};  // SignalHandler

}  // namespace Signal

#endif  // SIGNAL_HANDLER_H
