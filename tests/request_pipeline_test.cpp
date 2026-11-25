#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <mysql++/mysql++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "Connection.h"
#include "ContainerBridge.h"
#include "HandlerFactory.h"
#include "RequestHandler.h"
#include "RequestProcessor.h"
#include "ServerFunctor.h"
#include "sql/ConnectionPool.h"

namespace {

constexpr const char* kExpectedTestDbName = "auto_assignment_test";

// Lightweight stub handler to test Handler::execute(), requireField<T>(),
// ok(), error(), and exception-to-JSON mapping in isolation.
class StubEntityHandler : public AA::Server::Request::Handler {
 public:
  enum class Mode {
    kEchoPayload,
    kRequireIntId,
    kThrowRuntimeError,
    kThrowJsonOutOfRange
  };

  StubEntityHandler(nlohmann::json request, AA::Data::ConnectionPool& pool)
      : AA::Server::Request::Handler(std::move(request), pool) {}

  Mode mode{Mode::kEchoPayload};

 protected:
  std::string_view entityName() const override { return "stub_entity"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& /*conn*/) override {
    switch (mode) {
      case Mode::kEchoPayload:
        return ok(action, {{"received", payload}});
      case Mode::kRequireIntId: {
        const int id = requireField<int>(payload, "id");
        return ok(action, {{"id", id}});
      }
      case Mode::kThrowRuntimeError:
        throw std::runtime_error("Simulated database/domain error");
      case Mode::kThrowJsonOutOfRange:
        return payload.at("missing_required_key");
    }
    return error(action, "Unhandled test mode");
  }
};

// Allocates an ephemeral loopback TCP port so ServerFunctor tests do not
// collide with fixed ports across runs.
unsigned short findFreePort() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return 0;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(fd);
    return 0;
  }

  socklen_t len = sizeof(addr);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
    ::close(fd);
    return 0;
  }

  const unsigned short port = ntohs(addr.sin_port);
  ::close(fd);
  return port;
}

// Retries connecting until the background ServerFunctor thread finishes
// bind/listen.
std::optional<Connection::Connection> connectWithRetry(
    unsigned short port, int maxAttempts = 60,
    std::chrono::milliseconds delay = std::chrono::milliseconds(25)) {
  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    try {
      Connection::Connection candidate("127.0.0.1", port);
      if (candidate.isOpen()) {
        return candidate;
      }
    } catch (const Connection::NetworkError&) {
      std::this_thread::sleep_for(delay);
    }
  }
  return std::nullopt;
}

// Loads creds/dbcreds.txt when AA_ENABLE_DB_INTEGRATION_TESTS is defined,
// verifies it points to auto_assignment_test, and constructs ConnectionPool.
std::unique_ptr<AA::Data::ConnectionPool> createTestPool() {
#ifdef AA_ENABLE_DB_INTEGRATION_TESTS
  const std::filesystem::path credsPath =
      std::filesystem::path{"creds"} / "dbcreds.txt";
  AA::Data::ConnectionPool::Config cfg =
      AA::Data::ConnectionPool::loadConfigFromFile(credsPath);

  if (cfg.dbName.empty()) {
    cfg.dbName = kExpectedTestDbName;
  }

  if (cfg.dbName != kExpectedTestDbName) {
    throw std::runtime_error{
        "Refusing to run integration tests against non-test database: " +
        cfg.dbName};
  }

  return std::make_unique<AA::Data::ConnectionPool>(std::move(cfg));
#else
  return nullptr;
#endif
}

}  // namespace

// ==========================================
// 0. Live DB Credentials & Schema Check
// ==========================================
#ifdef AA_ENABLE_DB_INTEGRATION_TESTS
TEST(ConnectionPoolLiveDbTest, LoadsTestCredentialsAndInitializesSchema) {
  const std::filesystem::path credsPath =
      std::filesystem::path{"creds"} / "dbcreds.txt";
  ASSERT_TRUE(std::filesystem::exists(credsPath))
      << "Missing test credentials file at " << credsPath;

  const AA::Data::ConnectionPool::Config cfg =
      AA::Data::ConnectionPool::loadConfigFromFile(credsPath);
  EXPECT_EQ(cfg.server, "127.0.0.1");
  EXPECT_EQ(cfg.user, "aa_test");
  EXPECT_EQ(cfg.dbName, kExpectedTestDbName);
  EXPECT_FALSE(cfg.password.empty());

  auto pool = createTestPool();
  ASSERT_NE(pool, nullptr);

  mysqlpp::ScopedConnection conn(*pool, /*safe=*/true);
  ASSERT_TRUE(conn) << "Failed to acquire pooled connection to "
                    << kExpectedTestDbName;

  mysqlpp::Query q = conn->query("SELECT DATABASE()");
  const mysqlpp::StoreQueryResult res = q.store();
  ASSERT_EQ(res.num_rows(), 1U);
  EXPECT_EQ(std::string(res[0][0].c_str()), kExpectedTestDbName);
}
#endif

// ==========================================
// 1. Tests for Handler Base Class Contract
// ==========================================
TEST(HandlerBaseTest, ValidatesRequestEnvelopeAndPayloadType) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run Handler::execute() tests.";
  }

  // Non-object top-level JSON
  {
    StubEntityHandler handler{nlohmann::json::array({1, 2, 3}), *pool};
    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("data", ""), "stub_entity") << res.dump();
    EXPECT_EQ(res.value("action", ""), "unknown") << res.dump();
    const std::string msg = res.value("message", "");
    EXPECT_NE(msg.find("JSON object"), std::string::npos) << res.dump();
  }

  // Missing "action"
  {
    StubEntityHandler handler{nlohmann::json{{"data", "stub_entity"}}, *pool};
    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("data", ""), "stub_entity") << res.dump();
    EXPECT_EQ(res.value("action", ""), "unknown") << res.dump();
    const std::string msg = res.value("message", "");
    EXPECT_NE(msg.find("action"), std::string::npos) << res.dump();
  }

  // Non-object "payload"
  {
    StubEntityHandler handler{
        nlohmann::json{
            {"data", "stub_entity"}, {"action", "get"}, {"payload", "bad"}},
        *pool};
    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("data", ""), "stub_entity") << res.dump();
    EXPECT_EQ(res.value("action", ""), "get") << res.dump();
    const std::string msg = res.value("message", "");
    EXPECT_NE(msg.find("payload"), std::string::npos) << res.dump();
  }
}

TEST(HandlerBaseTest, FormatsOkAndMapsExceptionsToJsonError) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run Handler::execute() tests.";
  }

  // Happy path ok() formatting
  {
    StubEntityHandler handler{nlohmann::json{{"data", "stub_entity"},
                                             {"action", "ping"},
                                             {"payload", {{"hello", "world"}}}},
                              *pool};
    handler.mode = StubEntityHandler::Mode::kEchoPayload;

    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "ok") << res.dump();
    EXPECT_EQ(res.value("data", ""), "stub_entity") << res.dump();
    EXPECT_EQ(res.value("action", ""), "ping") << res.dump();
    ASSERT_TRUE(res.contains("result") && res["result"].is_object())
        << res.dump();
    ASSERT_TRUE(res["result"].contains("received") &&
                res["result"]["received"].is_object())
        << res.dump();
    EXPECT_EQ(res["result"]["received"].value("hello", ""), "world")
        << res.dump();
  }

  // requireField<int> missing key -> invalid_argument -> JSON error
  {
    StubEntityHandler handler{
        nlohmann::json{{"data", "stub_entity"},
                       {"action", "get"},
                       {"payload", nlohmann::json::object()}},
        *pool};
    handler.mode = StubEntityHandler::Mode::kRequireIntId;

    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("data", ""), "stub_entity") << res.dump();
    EXPECT_EQ(res.value("action", ""), "get") << res.dump();
    const std::string msg = res.value("message", "");
    EXPECT_NE(msg.find("id"), std::string::npos) << res.dump();
  }

  // std::runtime_error -> JSON error
  {
    StubEntityHandler handler{
        nlohmann::json{{"data", "stub_entity"},
                       {"action", "create"},
                       {"payload", nlohmann::json::object()}},
        *pool};
    handler.mode = StubEntityHandler::Mode::kThrowRuntimeError;

    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("message", ""), "Simulated database/domain error")
        << res.dump();
  }

  // nlohmann::json::out_of_range -> JSON error
  {
    StubEntityHandler handler{
        nlohmann::json{{"data", "stub_entity"},
                       {"action", "update"},
                       {"payload", nlohmann::json::object()}},
        *pool};
    handler.mode = StubEntityHandler::Mode::kThrowJsonOutOfRange;

    const nlohmann::json res = handler.execute();
    ASSERT_TRUE(res.is_object()) << res.dump();
    EXPECT_EQ(res.value("status", ""), "error") << res.dump();
    EXPECT_EQ(res.value("action", ""), "update") << res.dump();
    const std::string msg = res.value("message", "");
    EXPECT_NE(msg.find("JSON format/type error"), std::string::npos)
        << res.dump();
  }
}

// ==========================================
// 2. Tests for HandlerFactory::newHandler
// ==========================================
TEST(HandlerFactoryTest, RoutesAllFiveDomainEntities) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to instantiate HandlerFactory.";
  }

  const std::vector<std::string> entities = {"student", "teacher", "test",
                                             "school", "gradebook"};
  for (const auto& entity : entities) {
    nlohmann::json req{
        {"data", entity}, {"action", "get"}, {"payload", {{"id", 1}}}};
    std::unique_ptr<AA::Server::Request::Handler> handler;
    EXPECT_NO_THROW(handler = AA::Server::Request::HandlerFactory::newHandler(
                        std::move(req), *pool))
        << "Failed routing entity: " << entity;
    EXPECT_NE(handler, nullptr) << "Null handler returned for: " << entity;
  }
}

TEST(HandlerFactoryTest, RejectsInvalidOrUnknownRequests) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to instantiate HandlerFactory.";
  }

  // Unknown entity
  EXPECT_THROW(
      (void)AA::Server::Request::HandlerFactory::newHandler(
          nlohmann::json{{"data", "course"}, {"action", "get"}}, *pool),
      std::runtime_error);

  // Missing "data"
  EXPECT_THROW((void)AA::Server::Request::HandlerFactory::newHandler(
                   nlohmann::json{{"action", "get"}}, *pool),
               std::invalid_argument);

  // Empty "data" -> HandlerFactory throws std::runtime_error("Invalid handler
  // type: ")
  EXPECT_THROW((void)AA::Server::Request::HandlerFactory::newHandler(
                   nlohmann::json{{"data", ""}, {"action", "get"}}, *pool),
               std::runtime_error);

  // Missing "action"
  EXPECT_THROW((void)AA::Server::Request::HandlerFactory::newHandler(
                   nlohmann::json{{"data", "teacher"}}, *pool),
               std::invalid_argument);

  // Non-object request
  EXPECT_THROW((void)AA::Server::Request::HandlerFactory::newHandler(
                   nlohmann::json::array({"student", "get"}), *pool),
               std::invalid_argument);
}

// ==========================================
// 3. Tests for RequestProcessor
// ==========================================
TEST(RequestProcessorTest, ValidJsonRequestProducesStructuredResponse) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run RequestProcessor tests.";
  }

  AA::Server::RequestProcessor processor{*pool};
  const std::string rawReq =
      R"({"data":"student","action":"get","payload":{"id":42}})";
  const std::string rawReply = processor(rawReq);
  const nlohmann::json reply = nlohmann::json::parse(rawReply);

  EXPECT_EQ(reply.value("data", ""), "student") << rawReply;
  EXPECT_EQ(reply.value("action", ""), "get") << rawReply;
  const std::string status = reply.value("status", "");
  EXPECT_TRUE(status == "ok" || status == "error") << rawReply;
}

TEST(RequestProcessorTest, MalformedJsonReturnsParseErrorResponse) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run RequestProcessor tests.";
  }

  AA::Server::RequestProcessor processor{*pool};
  const std::string rawReply = processor(R"({"data":"student","action":)");
  const nlohmann::json reply = nlohmann::json::parse(rawReply);
  EXPECT_EQ(reply.value("status", ""), "error");
  EXPECT_EQ(reply.value("data", ""), "unknown");
  EXPECT_EQ(reply.value("action", ""), "parse");
  EXPECT_NE(reply.value("message", "").find("Invalid JSON"), std::string::npos);
}

TEST(RequestProcessorTest, RoutingAndRuntimeErrorsReturnErrorJson) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run RequestProcessor tests.";
  }

  AA::Server::RequestProcessor processor{*pool};

  // Unknown entity -> HandlerFactory throws std::runtime_error -> caught
  // cleanly
  {
    const std::string rawReply =
        processor(R"({"data":"unknown_entity","action":"get"})");
    const nlohmann::json reply = nlohmann::json::parse(rawReply);
    EXPECT_EQ(reply.value("status", ""), "error") << rawReply;
    EXPECT_NE(reply.value("message", "").find("Invalid handler type"),
              std::string::npos)
        << rawReply;
  }

  // Missing required routing keys -> HandlerFactory throws
  // std::invalid_argument
  {
    const std::string rawReply = processor(R"({"data":"teacher"})");
    const nlohmann::json reply = nlohmann::json::parse(rawReply);
    EXPECT_EQ(reply.value("status", ""), "error") << rawReply;
    EXPECT_NE(reply.value("message", "").find("action"), std::string::npos)
        << rawReply;
  }
}

TEST(RequestProcessorTest, HandlesConcurrentCallsAcrossThreads) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run concurrent RequestProcessor tests.";
  }

  AA::Server::RequestProcessor processor{*pool};
  constexpr int kNumThreads = 8;
  constexpr int kIterationsPerThread = 25;

  std::vector<std::jthread> workers;
  workers.reserve(kNumThreads);

  for (int t = 0; t < kNumThreads; ++t) {
    workers.emplace_back([&processor, t]() {
      for (int i = 0; i < kIterationsPerThread; ++i) {
        const std::string raw =
            (i % 4 == 0)
                ? "{malformed-json"
                : R"({"data":"student","action":"get","payload":{"id":)" +
                      std::to_string(t * 100 + i) + "}}";

        const std::string out = processor(raw);
        ASSERT_FALSE(out.empty());
        const nlohmann::json parsed = nlohmann::json::parse(out);
        EXPECT_TRUE(parsed.contains("status"));
      }
    });
  }
}

// ==========================================
// 4. Tests for ContainerBridge
// ==========================================
TEST(ContainerBridgeTest, HandlesValidJsonAndReplacesBuffer) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run ContainerBridge tests.";
  }

  int fds[2]{-1, -1};
  ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  ::close(fds[1]);

  auto conn = std::make_shared<Connection::Connection>(fds[0]);
  Connection::ConnectContainer container{
      R"({"data":"teacher","action":"get","payload":{"id":7}})", conn};

  AA::Server::ContainerBridge bridge{*pool};
  bridge(container);

  const nlohmann::json reply = nlohmann::json::parse(container.getBuffer());
  EXPECT_EQ(reply.value("data", ""), "teacher") << container.getBuffer();
  EXPECT_EQ(reply.value("action", ""), "get") << container.getBuffer();
  const std::string status = reply.value("status", "");
  EXPECT_TRUE(status == "ok" || status == "error") << container.getBuffer();
  EXPECT_EQ(container.getConn(), conn);
}

TEST(ContainerBridgeTest, HandlesMalformedJsonViaCallback) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run ContainerBridge tests.";
  }

  int fds[2]{-1, -1};
  ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  ::close(fds[1]);

  auto conn = std::make_shared<Connection::Connection>(fds[0]);
  Connection::ConnectContainer container{"not valid json{{{", conn};

  AA::Server::ContainerBridge bridge{*pool};
  auto callback = bridge.asCallback();
  callback(container);

  const nlohmann::json reply = nlohmann::json::parse(container.getBuffer());
  EXPECT_EQ(reply.value("status", ""), "error");
  EXPECT_EQ(reply.value("action", ""), "parse");
  EXPECT_NE(reply.value("message", "").find("Invalid JSON"), std::string::npos);
  EXPECT_EQ(container.getConn(), conn);
}

TEST(ContainerBridgeTest, HandlesExceptionsWithoutThrowingOutOfCallback) {
  auto pool = createTestPool();
  if (!pool) {
    GTEST_SKIP() << "Define AA_ENABLE_DB_INTEGRATION_TESTS with test DB config "
                    "to run ContainerBridge tests.";
  }

  int fds[2]{-1, -1};
  ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  ::close(fds[1]);

  auto conn = std::make_shared<Connection::Connection>(fds[0]);
  Connection::ConnectContainer container{
      R"({"data":"nonexistent_type","action":"create"})", conn};

  AA::Server::ContainerBridge bridge{*pool};
  auto callback = bridge.asCallback();

  EXPECT_NO_THROW(callback(container));
  const nlohmann::json reply = nlohmann::json::parse(container.getBuffer());
  EXPECT_EQ(reply.value("status", ""), "error");
  EXPECT_FALSE(reply.value("message", "").empty());
  EXPECT_EQ(container.getConn(), conn);
}

// ==========================================
// 5. Integration Test: ServerFunctor + ContainerBridge Loop
// ==========================================
TEST(ServerFunctorIntegrationTest, ProcessesClientFrameAndReturnsJsonReply) {
  const unsigned short port = findFreePort();
  ASSERT_NE(port, 0) << "Failed to allocate ephemeral test port";

  auto echoJsonCallback = [](Connection::ConnectContainer& container) {
    const nlohmann::json req = nlohmann::json::parse(container.getBuffer());
    const nlohmann::json res = {{"status", "ok"},
                                {"data", req.value("data", "unknown")},
                                {"action", req.value("action", "unknown")},
                                {"result", {{"echo", true}}}};
    container.setBuffer(res.dump());
  };

  std::thread serverThread([port, echoJsonCallback]() {
    try {
      AA::ServerFunctor server(port, "TestServer", echoJsonCallback,
                               /*writeNeeded=*/true);
      server();
    } catch (const std::exception& e) {
      std::cerr << "[INFO] Server thread exited: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "[INFO] Server thread exited with unknown exception\n";
    }
  });
  serverThread.detach();

  auto clientOpt = connectWithRetry(port);
  ASSERT_TRUE(clientOpt.has_value())
      << "Timed out waiting for TestServer on port " << port;
  Connection::Connection& client = *clientOpt;
  ASSERT_TRUE(client.isOpen());

  const nlohmann::json request = {
      {"data", "teacher"}, {"action", "get"}, {"payload", {{"id", 7}}}};
  ASSERT_TRUE(client.sx(request.dump()));
  const auto rawReply = client.rx();
  ASSERT_TRUE(rawReply.has_value());

  const nlohmann::json reply = nlohmann::json::parse(*rawReply);
  EXPECT_EQ(reply.value("status", ""), "ok");
  EXPECT_EQ(reply.value("data", ""), "teacher");
  EXPECT_EQ(reply.value("action", ""), "get");
  ASSERT_TRUE(reply.contains("result") && reply["result"].is_object());
  EXPECT_TRUE(reply["result"].value("echo", false));

  EXPECT_TRUE(client.sx("exit"));
  client.closeSocket();
}

TEST(ServerFunctorIntegrationTest, HandlesMultipleConcurrentClients) {
  const unsigned short port = findFreePort();
  ASSERT_NE(port, 0) << "Failed to allocate ephemeral test port";

  auto routerCallback = [](Connection::ConnectContainer& container) {
    const nlohmann::json req = nlohmann::json::parse(container.getBuffer());
    const nlohmann::json payload =
        req.value("payload", nlohmann::json::object());
    const nlohmann::json res = {{"status", "ok"},
                                {"data", req.value("data", "unknown")},
                                {"action", req.value("action", "unknown")},
                                {"result",
                                 {{"clientId", payload.value("clientId", -1)},
                                  {"seq", payload.value("seq", -1)}}}};
    container.setBuffer(res.dump());
  };

  std::thread serverThread([port, routerCallback]() {
    try {
      AA::ServerFunctor server(port, "ConcurrentTestServer", routerCallback,
                               /*writeNeeded=*/true);
      server();
    } catch (const std::exception& e) {
      std::cerr << "[INFO] Server thread exited: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "[INFO] Server thread exited with unknown exception\n";
    }
  });
  serverThread.detach();

  constexpr int kClientThreads = 6;
  constexpr int kMessagesPerClient = 10;
  std::atomic<int> successfulClients{0};

  std::vector<std::thread> clientThreads;
  clientThreads.reserve(kClientThreads);

  for (int c = 0; c < kClientThreads; ++c) {
    clientThreads.emplace_back([port, c, &successfulClients]() {
      try {
        auto clientOpt = connectWithRetry(port);
        if (!clientOpt.has_value() || !clientOpt->isOpen()) {
          ADD_FAILURE() << "Client " << c << " failed to connect";
          return;
        }
        Connection::Connection& client = *clientOpt;

        for (int seq = 0; seq < kMessagesPerClient; ++seq) {
          const nlohmann::json req = {
              {"data", (c % 2 == 0) ? "student" : "teacher"},
              {"action", "get"},
              {"payload", {{"clientId", c}, {"seq", seq}}}};

          if (!client.sx(req.dump())) {
            ADD_FAILURE() << "Client " << c << " sx() failed on seq " << seq;
            return;
          }

          const auto rawReply = client.rx();
          if (!rawReply.has_value()) {
            ADD_FAILURE() << "Client " << c << " rx() returned nullopt on seq "
                          << seq;
            return;
          }

          const nlohmann::json reply = nlohmann::json::parse(*rawReply);
          EXPECT_EQ(reply.value("status", ""), "ok");
          ASSERT_TRUE(reply.contains("result") && reply["result"].is_object());
          EXPECT_EQ(reply["result"].value("clientId", -1), c);
          EXPECT_EQ(reply["result"].value("seq", -1), seq);
        }

        EXPECT_TRUE(client.sx("exit"));
        client.closeSocket();
        ++successfulClients;
      } catch (const std::exception& e) {
        ADD_FAILURE() << "Client " << c << " threw exception: " << e.what();
      } catch (...) {
        ADD_FAILURE() << "Client " << c << " threw unknown exception";
      }
    });
  }

  for (auto& t : clientThreads) {
    if (t.joinable()) {
      t.join();
    }
  }

  EXPECT_EQ(successfulClients.load(), kClientThreads);
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
    if (test_info.result()->Skipped()) {
      std::cout << "[SKIPPED] Test: " << test_info.test_case_name() << "."
                << test_info.name() << "\n"
                << std::endl;
    } else if (test_info.result()->Passed()) {
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
  std::signal(SIGPIPE, SIG_IGN);
  mysqlpp::Connection::thread_start();

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::TestEventListeners& listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new VerboseTestLogger);

  const int result = RUN_ALL_TESTS();
  if (result == 0) {
    std::cout << "passed all tests" << std::endl;
  }

  mysqlpp::Connection::thread_end();
  return result;
}
