#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <array>
#include <csignal>
#include <functional>
#include <stdexcept>
#include <string_view>

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

  // Signal numbers are small fixed integers (< 64).
  // A fixed std::array avoids hash map overhead and lookup allocation.
  inline static std::array<std::function<void(int)>, 64> s_signal_registry{};

  // The centralized raw C-style function pointer expected by POSIX
  static void global_signal_router(int signum, [[maybe_unused]] siginfo_t* info,
                                   [[maybe_unused]] void* context) {
    if (signum >= 0 && static_cast<size_t>(signum) < s_signal_registry.size()) {
      if (auto& handler = s_signal_registry[signum]; handler) {
        // NOTE: Keep your lambda bodies lightweight (e.g., setting an atomic
        // flag) because complex operations inside signal handlers violate POSIX
        // safety rules.
        handler(signum);
      }
    }
  }

 public:
  SignalHandler(int signum, std::function<void(int)> handler)
      : m_signum{signum}, m_handler{std::move(handler)}, m_act{} {
    if (m_signum < 0 ||
        static_cast<size_t>(m_signum) >= s_signal_registry.size()) {
      throw SignalException{"**Invalid signal number**"};
    }

    // Register this instance's handler
    s_signal_registry[m_signum] = m_handler;

    m_act.sa_flags = SA_SIGINFO;
    m_act.sa_sigaction = &SignalHandler::global_signal_router;
    sigemptyset(&m_act.sa_mask);  // Ensure signal mask is clean
  }

  // Destructor: Clean up the registry to prevent dangling calls if the object
  // dies
  ~SignalHandler() {
    if (m_signum >= 0 &&
        static_cast<size_t>(m_signum) < s_signal_registry.size()) {
      s_signal_registry[m_signum] = nullptr;
    }
  }

  // Delete copy/move semantics to maintain stable static registry linkage
  SignalHandler(const SignalHandler&) = delete;
  SignalHandler& operator=(const SignalHandler&) = delete;
  SignalHandler(SignalHandler&&) = delete;
  SignalHandler& operator=(SignalHandler&&) = delete;

  void setUpSignalHandler() {
    if (sigaction(m_signum, &m_act, nullptr) == -1) {
      throw SignalException{"**Error setting up signal handler**"};
    }
  }
};  // class SignalHandler

}  // namespace Signal

#endif  // SIGNAL_HANDLER_H
