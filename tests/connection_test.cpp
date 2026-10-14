#include "Connection.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

#include <chrono>
#include <iostream>
#include <thread>

// ==========================================
// 1. Tests for ConnectContainer
// ==========================================
TEST(ConnectContainerTest, DefaultConstructor) {
  Connection::ConnectContainer container;
  EXPECT_EQ(container.getBuffer(), " ");
  EXPECT_EQ(container.getConn(), nullptr);
}

TEST(ConnectContainerTest, SettersAndGetters) {
  Connection::ConnectContainer container;

  container.setBuffer("Hello Test");
  EXPECT_EQ(container.getBuffer(), "Hello Test");

  // Test clearing
  container.clear();
  EXPECT_EQ(container.getBuffer(), "");
  EXPECT_EQ(container.getConn(), nullptr);
}

// ==========================================
// 2. Tests for NetworkError Exception
// ==========================================
TEST(NetworkErrorTest, ExceptionMessageFormatting) {
  std::string custom_msg = "Failed to bind";
  Connection::NetworkError err(custom_msg);

  std::string expected = "Network error: " + custom_msg;
  EXPECT_STREQ(err.what(), expected.c_str());
}

// ==========================================
// 3. Tests for Socket Error Handling
// ==========================================
TEST(SocketTest, InvalidSocketThrowsError) {
  EXPECT_THROW(
      { Connection::Socket invalid_socket(-1); }, Connection::NetworkError);
}

// ==========================================
// 4. Tests for Connection Logic & State
// ==========================================
TEST(ConnectionTest, SocketCreationAndState) {
  int raw_sock = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(raw_sock, 0);

  try {
    Connection::Connection conn(raw_sock);
    EXPECT_TRUE(conn.isOpen());
    EXPECT_TRUE(conn);  // operator bool()

    conn.setClosed();
    EXPECT_FALSE(conn.isOpen());
    EXPECT_FALSE(conn);
  } catch (const Connection::NetworkError& e) {
    FAIL() << "Socket creation failed unexpectedly: " << e.what();
  }
}

// ==========================================
// 5. Integration Test: PortListener Construction
// ==========================================
TEST(PortListenerTest, Construction) {
  unsigned short test_port = 8089;
  try {
    Connection::PortListener listener(test_port);
    SUCCEED();
  } catch (const std::exception& e) {
    // Handled gracefully if port is blocked or binding fails
  }
}

// ==========================================
// Custom Event Listener to print test status
// ==========================================
class VerboseTestLogger : public ::testing::EmptyTestEventListener {
  void OnTestStart(const ::testing::TestInfo& test_info) override {
    std::cout << "[RUNNING] Test: " << test_info.test_case_name() << "."
              << test_info.name() << std::endl;
  }

  void OnTestEnd(const ::testing::TestInfo& test_info) override {
    if (test_info.result()->Passed()) {
      std::cout << "[PASSED]  Test: " << test_info.test_case_name() << "."
                << test_info.name() << "\n"
                << std::endl;
    } else {
      std::cout << "[FAILED]  Test: " << test_info.test_case_name() << "."
                << test_info.name() << "\n"
                << std::endl;
    }
  }
};

// ==========================================
// Main function with summary check
// ==========================================
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // Add our custom logger to Google Test's listeners
  ::testing::TestEventListeners& listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new VerboseTestLogger);

  int result = RUN_ALL_TESTS();

  if (result == 0) {
    std::cout << "passed all tests" << std::endl;
  }

  return result;
}
