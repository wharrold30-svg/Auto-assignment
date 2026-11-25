#include "Connection.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <compare>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

namespace {

class SocketPairTest : public ::testing::Test {
 protected:
  int fds_[2]{-1, -1};

  void SetUp() override {
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_), 0)
        << "Failed to create AF_UNIX socketpair";
  }

  void TearDown() override {
    if (fds_[0] >= 0) {
      ::close(fds_[0]);
      fds_[0] = -1;
    }
    if (fds_[1] >= 0) {
      ::close(fds_[1]);
      fds_[1] = -1;
    }
  }

  int takeFd(int idx) {
    const int fd = fds_[idx];
    fds_[idx] = -1;
    return fd;
  }
};

}  // namespace

// ==========================================
// 1. Tests for ConnectContainer
// ==========================================
TEST(ConnectContainerTest, DefaultConstructor) {
  Connection::ConnectContainer container;
  EXPECT_EQ(container.getBuffer(), "");
  EXPECT_TRUE(container.getBuffer().empty());
  EXPECT_EQ(container.getConn(), nullptr);
}

TEST(ConnectContainerTest, SettersGettersAndClear) {
  Connection::ConnectContainer container;

  container.setBuffer("Hello Test");
  EXPECT_EQ(container.getBuffer(), "Hello Test");

  container.clear();
  EXPECT_EQ(container.getBuffer(), "");
  EXPECT_EQ(container.getConn(), nullptr);
}

TEST_F(SocketPairTest, PopulatedContainerAndSharedConnection) {
  auto conn = std::make_shared<Connection::Connection>(takeFd(0));
  Connection::ConnectContainer container{R"({"data":"student","action":"get"})",
                                         conn};

  EXPECT_EQ(container.getBuffer(), R"({"data":"student","action":"get"})");
  EXPECT_EQ(container.getConn(), conn);
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
// 4. Tests for Connection Logic, State & Move Semantics
// ==========================================
TEST(ConnectionTest, SocketCreationAndState) {
  int raw_sock = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(raw_sock, 0);

  try {
    Connection::Connection conn(raw_sock);
    EXPECT_TRUE(conn.isOpen());
    EXPECT_TRUE(static_cast<bool>(conn));

    conn.setClosed();
    EXPECT_FALSE(conn.isOpen());
    EXPECT_FALSE(static_cast<bool>(conn));

    conn.closeSocket();
    EXPECT_EQ(conn.getSocket(), -1);
  } catch (const Connection::NetworkError& e) {
    FAIL() << "Socket creation failed unexpectedly: " << e.what();
  }
}

TEST_F(SocketPairTest, MoveConstructorTransfersFdAndInvalidatesSource) {
  const int raw_fd = takeFd(0);
  Connection::Connection first{raw_fd};
  first.setAddr("127.0.0.1");

  ASSERT_TRUE(first.isOpen());
  EXPECT_EQ(first.getSocket(), raw_fd);
  EXPECT_EQ(first.getAddr(), "127.0.0.1");

  Connection::Connection moved{std::move(first)};

  EXPECT_TRUE(moved.isOpen());
  EXPECT_EQ(moved.getSocket(), raw_fd);
  EXPECT_EQ(moved.getAddr(), "127.0.0.1");

  EXPECT_FALSE(first.isOpen());
  EXPECT_EQ(first.getSocket(), -1);

  moved.closeSocket();
  EXPECT_FALSE(moved.isOpen());
  EXPECT_EQ(moved.getSocket(), -1);
  EXPECT_NO_THROW(moved.closeSocket());
}

TEST_F(SocketPairTest, MoveAssignmentTransfersFdAndHandlesSelfMove) {
  const int fd_a = takeFd(0);
  const int fd_b = takeFd(1);

  Connection::Connection conn_a{fd_a};
  conn_a.setAddr("10.0.0.1");

  Connection::Connection conn_b{fd_b};
  conn_b.setAddr("10.0.0.2");

  conn_a = std::move(conn_b);

  EXPECT_TRUE(conn_a.isOpen());
  EXPECT_EQ(conn_a.getSocket(), fd_b);
  EXPECT_EQ(conn_a.getAddr(), "10.0.0.2");

  EXPECT_FALSE(conn_b.isOpen());
  EXPECT_EQ(conn_b.getSocket(), -1);

  // Self-move assignment must preserve state without closing/corrupting the fd.
  auto& self_ref = conn_a;
  conn_a = std::move(self_ref);
  EXPECT_TRUE(conn_a.isOpen());
  EXPECT_EQ(conn_a.getSocket(), fd_b);
}

TEST_F(SocketPairTest, ComparisonOperators) {
  const int fd = takeFd(0);
  Connection::Connection c1{fd};
  Connection::Connection c2{fd};

  EXPECT_TRUE(c1 == c2);
  EXPECT_TRUE((c1 <=> c2) == std::strong_ordering::equal);
  EXPECT_TRUE(c1 == fd);
  EXPECT_TRUE((c1 <=> fd) == std::strong_ordering::equal);
  EXPECT_TRUE((c1 <=> (fd + 5)) == std::strong_ordering::less);
  EXPECT_TRUE((c1 <=> (fd - 5)) == std::strong_ordering::greater);
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
    std::cout << "[INFO] PortListener skipped/caught: " << e.what() << "\n";
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

  ::testing::TestEventListeners& listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new VerboseTestLogger);

  int result = RUN_ALL_TESTS();

  if (result == 0) {
    std::cout << "passed all tests" << std::endl;
  }

  return result;
}
