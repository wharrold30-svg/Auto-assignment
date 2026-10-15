#include "Email.h"

#include <gtest/gtest.h>

#include <iostream>
#include <type_traits>

class EmailTest : public ::testing::Test {
 protected:
  void SetUp() override { std::cout << "[SETUP] Initializing test case...\n"; }
  void TearDown() override {
    std::cout << "[TEARDOWN] Test case finished.\n\n";
  }
};

// 1. Test Deleted Copy and Move Operations
TEST_F(EmailTest, DeletedConstructorsAndOperators) {
  std::cout << "[TESTING] Part: Deleted Copy/Move Constructors & Assignment "
               "Operators\n";

  static_assert(!std::is_copy_constructible_v<Email::Email>,
                "Email should not be copy constructible");
  static_assert(!std::is_copy_assignable_v<Email::Email>,
                "Email should not be copy assignable");
  static_assert(!std::is_move_constructible_v<Email::Email>,
                "Email should not be move constructible");
  static_assert(!std::is_move_assignable_v<Email::Email>,
                "Email should not be move assignable");

  SUCCEED() << "[PASSED] Copy and move semantics are correctly deleted.";
}

// 2. Test Constructor without CC
TEST_F(EmailTest, ConstructorWithoutCc) {
  std::cout << "[TESTING] Part: Email Constructor (Without CC Optional)\n";

  try {
    Email::Email email("recipient@example.com", "sender@example.com",
                       "Sender Name", "Hello World", "This is a test body.");
    SUCCEED() << "[PASSED] Email successfully instantiated without CC.";
  } catch (const std::exception& e) {
    FAIL() << "[FAILED] Exception thrown during instantiation: " << e.what();
  }
}

// 3. Test Constructor with CC
TEST_F(EmailTest, ConstructorWithCc) {
  std::cout << "[TESTING] Part: Email Constructor (With CC Optional Vector)\n";

  std::vector<std::string> ccList = {"cc1@example.com", "cc2@example.com"};
  try {
    Email::Email email("recipient@example.com", "sender@example.com",
                       "Sender Name", "Hello World", "This is a test body.",
                       ccList);
    SUCCEED() << "[PASSED] Email successfully instantiated with CC list.";
  } catch (const std::exception& e) {
    FAIL() << "[FAILED] Exception thrown during instantiation with CC: "
           << e.what();
  }
}

// 4. Test dateTimeNow() public method
TEST_F(EmailTest, DateTimeNowMethod) {
  std::cout << "[TESTING] Part: dateTimeNow() Public Method\n";

  Email::Email email("recipient@example.com", "sender@example.com",
                     "Sender Name", "Hello World", "This is a test body.");

  std::string timestamp = email.dateTimeNow();
  EXPECT_FALSE(timestamp.empty());

  std::cout << "[INFO] Generated Timestamp: " << timestamp << "\n";
  SUCCEED()
      << "[PASSED] dateTimeNow() returned a valid, non-empty timestamp string.";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
