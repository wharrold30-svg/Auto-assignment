#include "Email.h"

#include <gtest/gtest.h>

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "Date.h"

// ============================================================================
// 1. MANUAL MOCK CLIENT (No <gmock/gmock.h> required!)
// ============================================================================
class ManualMockEmailClient : public Email::IEmailClient {
 public:
  // Lambda hook to inject custom behavior for `performSend` per test
  std::function<CURLcode(const std::string& url, const std::string& username,
                         const std::string& password,
                         const std::string& mailFrom,
                         const std::vector<std::string>& recipients,
                         curl_read_callback readCallback, void* userp)>
      sendHandler;

  CURLcode performSend(const std::string& url, const std::string& username,
                       const std::string& password, const std::string& mailFrom,
                       const std::vector<std::string>& recipients,
                       curl_read_callback readCallback, void* userp) override {
    if (sendHandler) {
      return sendHandler(url, username, password, mailFrom, recipients,
                         readCallback, userp);
    }
    return CURLE_OK;
  }
};

// ============================================================================
// 2. TESTABLE SUBCLASS (Exposes protected helper methods for testing)
// ============================================================================
class TestableEmail : public Email::Email {
 public:
  // Inherit constructors from Email base class
  using Email::Email;

  // Expose protected dateTimeNow() for direct verification
  std::string testDateTimeNow() const { return dateTimeNow(); }

  // Expose protected generateMessageId() for direct verification
  std::string testGenerateMessageId() const { return generateMessageId(); }
};

// ============================================================================
// 3. TEST FIXTURE SETUP / TEARDOWN
// ============================================================================
class EmailTestSuite : public ::testing::Test {
 protected:
  void SetUp() override {
    std::cout << "[SETUP] Initializing test case execution.\n";
  }

  void TearDown() override {
    std::cout << "[TEARDOWN] Test case "
                 "completed.\n----------------------------------------\n";
  }
};

// ============================================================================
// 4. TEST SUITE CASES
// ============================================================================

// TEST 1: Payload Generation & Formatting with Injected DT::Date
TEST_F(EmailTestSuite, TestPart_PayloadFormattingWithInjectedDate) {
  std::cout << "[TESTING PART]: RFC5322 payload formatting, headers, and "
               "DT::Date injection integration\n";

  auto mockClient = std::make_shared<ManualMockEmailClient>();

  mockClient->sendHandler = [](auto, auto, auto, auto, auto,
                               curl_read_callback cb, void* userp) {
    char buffer[4096];
    std::string fullPayload;
    std::size_t bytes = 0;
    while ((bytes = cb(buffer, 1, sizeof(buffer), userp)) > 0) {
      fullPayload.append(buffer, bytes);
    }

    // Verify payload contents and headers using standard string checks
    EXPECT_NE(fullPayload.find("Date: Wed, 18 Nov 2026"), std::string::npos);
    EXPECT_NE(fullPayload.find("To: <recipient@example.com>"),
              std::string::npos);
    EXPECT_NE(fullPayload.find("From: <sender@example.com>(Test Sender)"),
              std::string::npos);
    EXPECT_NE(fullPayload.find("Subject: Production-Ready Unit Test"),
              std::string::npos);
    EXPECT_NE(
        fullPayload.find("Testing email body content via mock injection."),
        std::string::npos);

    std::cout << "[RESULT] DT::Date injection & payload formatting test PASSED "
                 "successfully.\n";
    return CURLE_OK;
  };

  DT::Date fixedDate("Wed, 18 Nov 2026 14:30:00 +0000");
  auto dateProvider = [fixedDate]() { return fixedDate; };
  auto idProvider = []() { return "static-msg-id-456"; };

  Email::Email email("recipient@example.com", "sender@example.com",
                     "Test Sender", "Production-Ready Unit Test",
                     "Testing email body content via mock injection.",
                     std::nullopt, dateProvider, idProvider, mockClient);

  EXPECT_NO_THROW(
      { email.send("smtps://smtp.server.com:465", "user", "pass"); });
}

// TEST 2: Multiple CC Recipients Comma Separation
TEST_F(EmailTestSuite, TestPart_MultipleCcRecipientsFormatting) {
  std::cout << "[TESTING PART]: Comma separation and formatting for multiple "
               "CC recipients\n";

  auto mockClient = std::make_shared<ManualMockEmailClient>();

  mockClient->sendHandler = [](auto, auto, auto, auto, auto,
                               curl_read_callback cb, void* userp) {
    char buffer[4096];
    std::string fullPayload;
    std::size_t bytes = 0;
    while ((bytes = cb(buffer, 1, sizeof(buffer), userp)) > 0) {
      fullPayload.append(buffer, bytes);
    }

    // Verify multiple CC header formatting
    EXPECT_NE(
        fullPayload.find(
            "Cc: alice@example.com, bob@example.com, charlie@example.com"),
        std::string::npos);

    std::cout << "[RESULT] Multiple CC formatting test PASSED successfully.\n";
    return CURLE_OK;
  };

  std::vector<std::string> multipleCc = {"alice@example.com", "bob@example.com",
                                         "charlie@example.com"};

  Email::Email email(
      "to@example.com", "from@example.com", "Sender", "Subject", "Body",
      multipleCc, []() { return DT::Date(); }, []() { return "id"; },
      mockClient);

  EXPECT_NO_THROW({ email.send("smtps://smtp.server.com", "u", "p"); });
}

// TEST 3: Network Failure Exception Handling
TEST_F(EmailTestSuite, TestPart_CurlErrorHandlingAndExceptions) {
  std::cout
      << "[TESTING PART]: Exception handling when libcurl fails to transmit\n";

  auto mockClient = std::make_shared<ManualMockEmailClient>();

  mockClient->sendHandler = [](auto, auto, auto, auto, auto, auto, auto) {
    return CURLE_SEND_ERROR;  // Simulate send failure
  };

  DT::Date fixedDate;
  Email::Email email(
      "to@example.com", "from@example.com", "Sender", "Subject", "Body",
      std::nullopt, [fixedDate]() { return fixedDate; }, []() { return "id"; },
      mockClient);

  EXPECT_THROW(
      { email.send("smtps://bad-server.com", "u", "p"); }, std::runtime_error);

  std::cout << "[RESULT] Exception handling test PASSED successfully.\n";
}

// TEST 4: Real Protected dateTimeNow() Format Validation
TEST_F(EmailTestSuite, TestPart_RealDateTimeNowFormat) {
  std::cout << "[TESTING PART]: Real dateTimeNow() implementation and RFC5322 "
               "string structure\n";

  TestableEmail email("to@example.com", "from@example.com", "Sender", "Subject",
                      "Body");

  std::string timeStr = email.testDateTimeNow();

  EXPECT_FALSE(timeStr.empty());
  EXPECT_NE(timeStr.find(':'), std::string::npos);

  std::cout << "[RESULT] Generated timestamp: " << timeStr << "\n";
  std::cout << "[RESULT] dateTimeNow() test PASSED successfully.\n";
}

// TEST 5: Real Protected generateMessageId() Structure Validation
TEST_F(EmailTestSuite, TestPart_RealGenerateMessageIdStructure) {
  std::cout << "[TESTING PART]: Real generateMessageId() implementation length "
               "and uniqueness\n";

  TestableEmail email("to@example.com", "from@example.com", "Sender", "Subject",
                      "Body");

  std::string id1 = email.testGenerateMessageId();
  std::string id2 = email.testGenerateMessageId();

  // Validate length (MESSAGE_ID_LEN = 37) and prefix separator
  EXPECT_EQ(id1.size(), 37);
  EXPECT_EQ(id2.size(), 37);
  EXPECT_EQ(id1[14], '.');
  EXPECT_NE(id1, id2);

  std::cout << "[RESULT] Generated Message-ID 1: " << id1 << "\n";
  std::cout << "[RESULT] Generated Message-ID 2: " << id2 << "\n";
  std::cout << "[RESULT] generateMessageId() test PASSED successfully.\n";
}
