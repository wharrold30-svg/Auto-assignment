#include <gtest/gtest.h>

#include <atomic>
#include <csignal>
#include <memory>

#include "SignalHandler.h"

class SignalHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset or prepare state before each test if necessary
  }

  void TearDown() override {
    // Clean up after each test if necessary
  }
};

// 1. Test successful signal setup, triggering, and execution of lambda
TEST_F(SignalHandlerTest, CatchesAndHandlesSignalSuccessfully) {
  std::atomic<bool> signal_handled{false};
  std::atomic<int> received_signal{0};

  {
    // Use std::unique_ptr to respect deleted copy/move semantics
    // and safely manage scope lifetime.
    auto handler = std::make_unique<Signal::SignalHandler>(
        SIGUSR1, [&signal_handled, &received_signal](int signum) {
          received_signal = signum;
          signal_handled = true;
        });

    // Verify setup succeeds without throwing
    ASSERT_NO_THROW(handler->setUpSignalHandler());

    // Raise the signal programmatically
    std::raise(SIGUSR1);

    // Verify our handler was invoked correctly
    EXPECT_TRUE(signal_handled.load());
    EXPECT_EQ(received_signal.load(), SIGUSR1);
  }  // <- handler goes out of scope here, triggering the destructor to
     // unregister safely
}

// 2. Test RAII cleanup (ensuring object destruction unregisters the handler
// safely)
TEST_F(SignalHandlerTest, DestructorUnregistersHandlerSafely) {
  {
    auto handler = std::make_unique<Signal::SignalHandler>(SIGUSR2, [](int) {
      FAIL() << "Handler should not be called after object destruction!";
    });
    handler->setUpSignalHandler();
  }  // handler is destroyed here, clearing s_signal_registry[SIGUSR2]

  // Raising the signal now should invoke the default system behavior or be
  // ignored, rather than executing the dead lambda or crashing.
  EXPECT_NO_THROW(std::raise(SIGUSR2));
}

// 3. Test bounds checking for invalid signal numbers (negative or out-of-range)
TEST_F(SignalHandlerTest, ThrowsExceptionOnInvalidSignalNumbers) {
  // Negative signal number should throw SignalException
  EXPECT_THROW(
      {
        Signal::SignalHandler negative_handler(-5, [](int) {});
        negative_handler.setUpSignalHandler();
      },
      Signal::SignalException);

  // Out-of-bounds signal number (>= 64 based on std::array size) should throw
  EXPECT_THROW(
      {
        Signal::SignalHandler out_of_bounds_handler(999, [](int) {});
        out_of_bounds_handler.setUpSignalHandler();
      },
      Signal::SignalException);
}

// 4. Test OS-level error handling when sigaction fails (e.g., trying to catch
// SIGKILL)
TEST_F(SignalHandlerTest, ThrowsExceptionOnUncatchableSystemSignal) {
  // SIGKILL cannot be caught or ignored under POSIX systems;
  // sigaction will return -1, triggering the exception in setUpSignalHandler().
  auto handler = std::make_unique<Signal::SignalHandler>(SIGKILL, [](int) {});

  EXPECT_THROW({ handler->setUpSignalHandler(); }, Signal::SignalException);
}

// 5. Test multiple distinct signal handlers coexisting safely
TEST_F(SignalHandlerTest, MultipleHandlersCoexist) {
  std::atomic<bool> sig1_hit{false};
  std::atomic<bool> sig2_hit{false};

  auto handler1 = std::make_unique<Signal::SignalHandler>(
      SIGUSR1, [&](int) { sig1_hit = true; });
  auto handler2 = std::make_unique<Signal::SignalHandler>(
      SIGUSR2, [&](int) { sig2_hit = true; });

  handler1->setUpSignalHandler();
  handler2->setUpSignalHandler();

  std::raise(SIGUSR1);
  EXPECT_TRUE(sig1_hit.load());
  EXPECT_FALSE(sig2_hit.load());

  std::raise(SIGUSR2);
  EXPECT_TRUE(sig2_hit.load());
}
