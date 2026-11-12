#include "Log.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// Helper function to safely remove test files/directories
void cleanupLogFile(const std::filesystem::path& filepath) {
  std::error_code ec;
  if (std::filesystem::exists(filepath, ec)) {
    std::filesystem::remove(filepath, ec);
  }
}

// ==========================================
// 1. Core Log Class Tests (Custom Path)
// ==========================================
TEST(LogCoreTest, CustomPathAndException) {
  std::cout << "[TEST START] Testing Log::Log core class with custom file path "
               "and exception behavior...\n";

  std::string testFilePath = "test_logs/custom_test.log";
  cleanupLogFile(testFilePath);

  Log::Log customLog{testFilePath};
  std::string testMessage = "Unit test core log message";

  // Verify that calling the operator logs to file and throws std::runtime_error
  EXPECT_THROW({ customLog(testMessage); }, std::runtime_error);

  // Verify file creation and content
  ASSERT_TRUE(std::filesystem::exists(testFilePath))
      << "Log file should be created.";

  std::ifstream inFile(testFilePath);
  ASSERT_TRUE(inFile.is_open())
      << "Should be able to open the created log file.";

  std::string line;
  std::getline(inFile, line);
  inFile.close();

  // Check that the line contains our message (format includes date | msg)
  EXPECT_NE(line.find(testMessage), std::string::npos)
      << "Log file content should contain the logged message.";

  // Cleanup
  cleanupLogFile(testFilePath);
  std::filesystem::remove_all("test_logs");

  std::cout
      << "[TEST PASSED] Log::Log core class test passed successfully!\n\n";
}

// ==========================================
// 2. Connection Error Log Tests
// ==========================================
TEST(ConnectionLogTest, ConnectionErrorLogging) {
  std::cout
      << "[TEST START] Testing Connection::ConnectionErrorLog functor...\n";

  std::string logPath = "log/error/connection.log";
  cleanupLogFile(logPath);

  std::string msg = "Connection timeout error occurred";

  EXPECT_THROW({ Connection::ConnectionErrorLog(msg); }, std::runtime_error);

  ASSERT_TRUE(std::filesystem::exists(logPath))
      << "Connection log file must be created.";

  cleanupLogFile(logPath);
  std::cout << "[TEST PASSED] Connection::ConnectionErrorLog test passed "
               "successfully!\n\n";
}

// ==========================================
// 3. Email Error Log Tests
// ==========================================
TEST(EmailLogTest, EmailErrorLogging) {
  std::cout << "[TEST START] Testing Email::EmailErrorLog functor...\n";

  std::string logPath = "log/error/email.log";
  cleanupLogFile(logPath);

  std::string msg = "Failed to send SMTP email";

  EXPECT_THROW({ Email::EmailErrorLog(msg); }, std::runtime_error);

  ASSERT_TRUE(std::filesystem::exists(logPath))
      << "Email log file must be created.";

  cleanupLogFile(logPath);
  std::cout
      << "[TEST PASSED] Email::EmailErrorLog test passed successfully!\n\n";
}

// ==========================================
// 4. Database Error Log Tests
// ==========================================
TEST(DatabaseLogTest, DatabaseErrorLogging) {
  std::cout << "[TEST START] Testing Database::DatabaseErrorLog functor...\n";

  std::string logPath = "log/error/database.log";
  cleanupLogFile(logPath);

  std::string msg = "Database connection connection dropped";

  EXPECT_THROW({ Database::DatabaseErrorLog(msg); }, std::runtime_error);

  ASSERT_TRUE(std::filesystem::exists(logPath))
      << "Database log file must be created.";

  cleanupLogFile(logPath);
  std::cout << "[TEST PASSED] Database::DatabaseErrorLog test passed "
               "successfully!\n\n";
}

// ==========================================
// 5. AA Namespace (Save & Load Error) Tests
// ==========================================
TEST(AALogTest, SaveAndLoadErrorLogging) {
  std::cout << "[TEST START] Testing AA::SaveErrorLog and AA::LoadErrorLog "
               "functors...\n";

  std::string logPath = "log/error/database.log";

  // Test SaveErrorLog
  {
    cleanupLogFile(logPath);
    std::string msg = "Failed to save state";
    EXPECT_THROW({ AA::SaveErrorLog(msg); }, std::runtime_error);
    ASSERT_TRUE(std::filesystem::exists(logPath))
        << "SaveErrorLog should write to database.log.";
  }

  // Test LoadErrorLog
  {
    cleanupLogFile(logPath);
    std::string msg = "Failed to load state";
    EXPECT_THROW({ AA::LoadErrorLog(msg); }, std::runtime_error);
    ASSERT_TRUE(std::filesystem::exists(logPath))
        << "LoadErrorLog should write to database.log.";
  }

  cleanupLogFile(logPath);
  std::cout << "[TEST PASSED] AA namespace log tests passed successfully!\n\n";
}
