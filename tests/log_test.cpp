#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// Include the header under test
#include "Log.h"

class LogTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clean up any pre-existing test log directories to ensure a clean state
    if (std::filesystem::exists("log")) {
      std::filesystem::remove_all("log");
    }
    if (std::filesystem::exists("test_custom.log")) {
      std::filesystem::remove("test_custom.log");
    }
  }

  void TearDown() override {
    // Clean up after tests
    if (std::filesystem::exists("log")) {
      std::filesystem::remove_all("log");
    }
    if (std::filesystem::exists("test_custom.log")) {
      std::filesystem::remove("test_custom.log");
    }
  }
};

// 1. Testing Base Log Class Functionality
TEST_F(LogTest, BaseLogWritesCorrectly) {
  std::cout
      << "[TESTING] Part: Base Log::Log class file writing and formatting...\n";

  std::string testFile = "test_custom.log";
  std::string testMessage = "This is a custom base log test message.";

  {
    Log::Log logger(testFile);
    logger(testMessage);
  }  // Logger goes out of scope and closes the file handle

  // Verify file exists
  ASSERT_TRUE(std::filesystem::exists(testFile))
      << "[FAILED] Base Log file was not created.";
  std::cout << "[PASSED] Base Log file created successfully.\n";

  // Verify content
  std::ifstream inFile(testFile);
  std::string line;
  std::getline(inFile, line);

  // The log format is: "Date | message"
  // We check if the message is contained in the written line
  EXPECT_NE(line.find(testMessage), std::string::npos)
      << "[FAILED] Log message not found in file content.";
  std::cout << "[PASSED] Base Log message successfully written and verified.\n";
}

// 2. Testing ConnectionErrorLog
TEST_F(LogTest, ConnectionErrorLogCreatesAndWrites) {
  std::cout << "[TESTING] Part: Connection::ConnectionErrorLog default path "
               "and logging...\n";

  std::string expectedPath = "log/error/connection.log";
  std::string testMessage = "Database socket timeout detected.";

  {
    Connection::ConnectionErrorLog connLog(testMessage);
  }

  ASSERT_TRUE(std::filesystem::exists(expectedPath))
      << "[FAILED] Connection error log file not created at " << expectedPath;
  std::cout << "[PASSED] Connection error log file created at correct path.\n";

  std::ifstream inFile(expectedPath);
  std::string line;
  std::getline(inFile, line);

  EXPECT_NE(line.find(testMessage), std::string::npos)
      << "[FAILED] Connection error message missing.";
  std::cout << "[PASSED] Connection error message verified successfully.\n";
}

// 3. Testing EmailErrorLog
TEST_F(LogTest, EmailErrorLogCreatesAndWrites) {
  std::cout
      << "[TESTING] Part: Email::EmailErrorLog default path and logging...\n";

  std::string expectedPath = "log/error/email.log";
  std::string testMessage = "SMTP authentication failed.";

  {
    Email::EmailErrorLog emailLog(testMessage);
  }

  ASSERT_TRUE(std::filesystem::exists(expectedPath))
      << "[FAILED] Email error log file not created at " << expectedPath;
  std::cout << "[PASSED] Email error log file created at correct path.\n";

  std::ifstream inFile(expectedPath);
  std::string line;
  std::getline(inFile, line);

  EXPECT_NE(line.find(testMessage), std::string::npos)
      << "[FAILED] Email error message missing.";
  std::cout << "[PASSED] Email error message verified successfully.\n";
}

// 4. Testing DatabaseErrorLog
TEST_F(LogTest, DatabaseErrorLogCreatesAndWrites) {
  std::cout << "[TESTING] Part: Database::DatabaseErrorLog default path and "
               "logging...\n";

  std::string expectedPath = "log/error/database.log";
  std::string testMessage = "Query execution timeout.";

  {
    Database::DatabaseErrorLog dbLog(testMessage);
  }

  ASSERT_TRUE(std::filesystem::exists(expectedPath))
      << "[FAILED] Database error log file not created at " << expectedPath;
  std::cout << "[PASSED] Database error log file created at correct path.\n";

  std::ifstream inFile(expectedPath);
  std::string line;
  std::getline(inFile, line);

  EXPECT_NE(line.find(testMessage), std::string::npos)
      << "[FAILED] Database error message missing.";
  std::cout << "[PASSED] Database error message verified successfully.\n";
}

// Main function to run all tests
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  std::cout << "=== STARTING LOG.H UNIT TESTS ===\n";
  int result = RUN_ALL_TESTS();
  std::cout << "=== FINISHED LOG.H UNIT TESTS ===\n";
  return result;
}
