#include <gtest/gtest.h>

#include <atomic>
#include <csignal>

#include "SignalHandler.h"

// Atomic flag used to safely capture signal execution across threads/contexts
std::atomic<int> g_received_signal{0};

// ============================================================================
// Test 1: Exception Handling
// What it tests: Ensures that `setUpSignalHandler()` correctly detects invalid
// signal numbers, invokes `sigaction` failure paths, and throws
// `SignalException`.
// ============================================================================
TEST(SignalHandlerTest, InvalidSignalThrowsException) {
  // 9999 is an invalid signal number, guaranteed to cause sigaction() to fail.
  Signal::SignalHandler handler(9999, [](int) { g_received_signal = -1; });

  EXPECT_THROW({ handler.setUpSignalHandler(); }, Signal::SignalException);
}

// ============================================================================
// Test 2: Signal Routing and Callback Execution
// What it tests: Verifies successful signal registration, routing through the
// static `global_signal_router`, and proper invocation of the user's
// `std::function`.
// ============================================================================
TEST(SignalHandlerTest, ValidSignalTriggersCallback) {
  g_received_signal = 0;

  // Register a modern lambda handler for SIGUSR1 (safe for user-defined
  // testing)
  Signal::SignalHandler handler(SIGUSR1,
                                [](int signum) { g_received_signal = signum; });

  // Ensure setup succeeds without throwing
  ASSERT_NO_THROW({ handler.setUpSignalHandler(); });

  // Trigger the signal programmatically
  std::raise(SIGUSR1);

  // Verify the callback was executed and received the correct signal number
  EXPECT_EQ(g_received_signal.load(), SIGUSR1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
