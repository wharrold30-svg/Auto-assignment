#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <format>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ServerFunctor.h"

namespace {
using namespace std::chrono_literals;

class TestPrinter : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo& info) override {
    std::cout << std::format("\n[TESTING ] {}.{}\n", info.test_suite_name(),
                             info.name());
  }
  void OnTestEnd(const ::testing::TestInfo& info) override {
    std::cout << std::format("[{}  ] {}.{}\n",
                             info.result()->Passed() ? "PASSED" : "FAILED",
                             info.test_suite_name(), info.name());
  }
};

unsigned short getFreePort() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_GE(sock, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  EXPECT_EQ(bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
  socklen_t len = sizeof(addr);
  EXPECT_EQ(getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len), 0);
  unsigned short port = ntohs(addr.sin_port);
  EXPECT_EQ(close(sock), 0);
  return port;
}

class TestClient {
 public:
  TestClient() : m_fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (m_fd >= 0) {
      timeval tv{2, 0};
      EXPECT_EQ(setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)), 0);
      EXPECT_EQ(setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)), 0);
    }
  }
  ~TestClient() {
    if (m_fd >= 0) static_cast<void>(close(m_fd));
  }
  TestClient(const TestClient&) = delete;
  TestClient& operator=(const TestClient&) = delete;

  bool connectTo(unsigned short port) {
    if (m_fd < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) return false;
    for (int i = 0; i < 20; ++i) {
      if (connect(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
        return true;
      std::this_thread::sleep_for(25ms);
    }
    return false;
  }

  bool sendMsg(const std::string& msg) const {
    uint32_t netLen = htonl(static_cast<uint32_t>(msg.size()));
    if (send(m_fd, &netLen, sizeof(netLen), MSG_NOSIGNAL) != sizeof(netLen))
      return false;
    return send(m_fd, msg.data(), msg.size(), MSG_NOSIGNAL) ==
           static_cast<ssize_t>(msg.size());
  }

  std::string recvMsg() const {
    uint32_t netLen = 0;
    if (recv(m_fd, &netLen, sizeof(netLen), MSG_WAITALL) != sizeof(netLen))
      return "";
    uint32_t len = ntohl(netLen);
    std::string buf(len, '\0');
    if (recv(m_fd, buf.data(), len, MSG_WAITALL) != static_cast<ssize_t>(len))
      return "";
    return buf;
  }

  bool waitForClose() const {
    char c{};
    return recv(m_fd, &c, 1, 0) == 0;
  }

 private:
  int m_fd{-1};
};

class ServerFunctorTest : public ::testing::Test {
 protected:
  void SetUp() override { m_port = getFreePort(); }

  void startServer(std::function<void(Connection::ConnectContainer&)> fn,
                   bool writeNeeded = true,
                   std::string_view name = "TestServer") {
    auto ready = std::make_shared<std::promise<void>>();
    auto fut = ready->get_future();
    std::thread([p = m_port, n = std::string(name), f = std::move(fn),
                 writeNeeded, ready]() mutable {
      try {
        AA::ServerFunctor srv(p, n, std::move(f), writeNeeded);
        ready->set_value();
        srv();
      } catch (...) {
        try {
          ready->set_exception(std::current_exception());
        } catch (...) {
        }
      }
    }).detach();
    fut.get();
    std::this_thread::sleep_for(20ms);
  }

  unsigned short m_port{0};
};

TEST_F(ServerFunctorTest, InitializationAndPortConflict) {
  std::cout << "  -> Testing: Constructor binding, banner output, and port "
               "collision error\n";
  testing::internal::CaptureStdout();
  startServer([](Connection::ConnectContainer&) {}, true, "AlphaServer");
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_NE(out.find("AlphaServer"), std::string::npos);
  EXPECT_NE(out.find(std::to_string(m_port)), std::string::npos);
  EXPECT_THROW((AA::ServerFunctor{m_port, "Dup",
                                  [](Connection::ConnectContainer&) {}, true}),
               Connection::NetworkError);
}

TEST_F(ServerFunctorTest, SingleClientAcceptProcessAndReply) {
  std::cout << "  -> Testing: Single client accept, callback processing, and "
               "write queue\n";
  startServer(
      [](Connection::ConnectContainer& c) {
        c.setBuffer("ACK:" + c.getBuffer());
      },
      true);
  TestClient client;
  ASSERT_TRUE(client.connectTo(m_port));
  ASSERT_TRUE(client.sendMsg("Hello"));
  EXPECT_EQ(client.recvMsg(), "ACK:Hello");
}

TEST_F(ServerFunctorTest, FireAndForgetSuppressesWriteQueue) {
  std::cout << "  -> Testing: m_writeNeeded = false suppresses response "
               "transmission\n";
  auto prom = std::make_shared<std::promise<std::string>>();
  auto fut = prom->get_future();
  auto flag = std::make_shared<std::once_flag>();

  startServer(
      [prom, flag](Connection::ConnectContainer& c) {
        std::call_once(*flag, [&]() { prom->set_value(c.getBuffer()); });
        c.setBuffer("IGNORED");
      },
      false);

  TestClient client;
  ASSERT_TRUE(client.connectTo(m_port));
  ASSERT_TRUE(client.sendMsg("OneWay"));
  ASSERT_EQ(fut.wait_for(2s), std::future_status::ready);
  EXPECT_EQ(fut.get(), "OneWay");
  EXPECT_TRUE(client.recvMsg().empty());
}

TEST_F(ServerFunctorTest, ExitCommandDisconnectsAndUpdatesMaxFd) {
  std::cout << "  -> Testing: 'exit' payload closes socket, clears fd_set, and "
               "updates m_maxFd\n";
  auto calls = std::make_shared<std::atomic<int>>(0);
  startServer(
      [calls](Connection::ConnectContainer& c) {
        ++(*calls);
        c.setBuffer("OK");
      },
      true);

  TestClient c1;
  ASSERT_TRUE(c1.connectTo(m_port));
  ASSERT_TRUE(c1.sendMsg("ping"));
  EXPECT_EQ(c1.recvMsg(), "OK");
  ASSERT_TRUE(c1.sendMsg("exit"));
  EXPECT_TRUE(c1.waitForClose());
  EXPECT_EQ(calls->load(), 1);

  TestClient c2;
  ASSERT_TRUE(c2.connectTo(m_port));
  ASSERT_TRUE(c2.sendMsg("after"));
  EXPECT_EQ(c2.recvMsg(), "OK");
}

TEST_F(ServerFunctorTest, AbruptDisconnectCleanup) {
  std::cout
      << "  -> Testing: Abrupt client disconnect (EOF) keeps server healthy\n";
  startServer(
      [](Connection::ConnectContainer& c) {
        c.setBuffer("ECHO:" + c.getBuffer());
      },
      true);
  {
    TestClient temp;
    ASSERT_TRUE(temp.connectTo(m_port));
  }
  std::this_thread::sleep_for(50ms);

  TestClient survivor;
  ASSERT_TRUE(survivor.connectTo(m_port));
  ASSERT_TRUE(survivor.sendMsg("Alive"));
  EXPECT_EQ(survivor.recvMsg(), "ECHO:Alive");
}

TEST_F(ServerFunctorTest, ConcurrentMultiClientMultiplexing) {
  std::cout << "  -> Testing: Multi-client select() multiplexing and staggered "
               "disconnects\n";
  constexpr int kClients = 12;
  auto mtx = std::make_shared<std::mutex>();
  auto count = std::make_shared<int>(0);

  startServer(
      [mtx, count](Connection::ConnectContainer& c) {
        {
          std::lock_guard lock(*mtx);
          ++(*count);
        }
        c.setBuffer("RESP_" + c.getBuffer());
      },
      true);

  std::vector<TestClient> clients(kClients);
  for (int i = 0; i < kClients; ++i) ASSERT_TRUE(clients[i].connectTo(m_port));

  std::vector<std::thread> workers;
  for (int i = 0; i < kClients; ++i) {
    workers.emplace_back([&clients, i]() {
      std::string msg = std::format("Client_{}", i);
      ASSERT_TRUE(clients[i].sendMsg(msg));
      EXPECT_EQ(clients[i].recvMsg(), "RESP_" + msg);
    });
  }
  for (auto& t : workers) t.join();
  EXPECT_EQ(*count, kClients);

  for (int i = kClients - 1; i >= kClients / 2; --i) {
    ASSERT_TRUE(clients[i].sendMsg("exit"));
    EXPECT_TRUE(clients[i].waitForClose());
  }
  for (int i = 0; i < kClients / 2; ++i) {
    std::string msg = std::format("R2_{}", i);
    ASSERT_TRUE(clients[i].sendMsg(msg));
    EXPECT_EQ(clients[i].recvMsg(), "RESP_" + msg);
  }
}

TEST_F(ServerFunctorTest, AssignsClientIpOnAccept) {
  std::cout
      << "  -> Testing: Client IP address assignment (127.0.0.1) on accept\n";
  auto ipProm = std::make_shared<std::promise<std::string>>();
  auto ipFut = ipProm->get_future();
  auto flag = std::make_shared<std::once_flag>();

  startServer(
      [ipProm, flag](Connection::ConnectContainer& c) {
        std::call_once(*flag,
                       [&]() { ipProm->set_value(c.getConn()->getAddr()); });
        c.setBuffer("IP_OK");
      },
      true);

  TestClient client;
  ASSERT_TRUE(client.connectTo(m_port));
  ASSERT_TRUE(client.sendMsg("ip"));
  EXPECT_EQ(client.recvMsg(), "IP_OK");
  ASSERT_EQ(ipFut.wait_for(2s), std::future_status::ready);
  EXPECT_EQ(ipFut.get(), "127.0.0.1");
}
}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::UnitTest::GetInstance()->listeners().Append(new TestPrinter);
  return RUN_ALL_TESTS();
}
