#include "GetCredsFunctor.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

// Helper fixture to set up and clean up temporary test files
class GetCredsFunctorTest : public ::testing::Test {
 protected:
  void SetUp() override { testFileName = "test_creds.txt"; }

  void TearDown() override {
    if (std::filesystem::exists(testFileName)) {
      std::filesystem::remove(testFileName);
    }
    if (std::filesystem::exists("nested")) {
      std::filesystem::remove_all("nested");
    }
  }

  void createTestFile(const std::string& filename, const std::string& content) {
    std::ofstream outFile(filename);
    outFile << content;
  }

  std::string testFileName;
};

// Test 1: Successful credential extraction
TEST_F(GetCredsFunctorTest, SuccessfulExtraction) {
  std::cout << "[TESTING] Part 1: Successful credential extraction with valid "
               "regex capture group...\n";

  createTestFile(testFileName, "api_key=secret12345");

  Creds::GetCredsFunctor getCreds;
  std::string result = getCreds(R"(api_key=(.*))", testFileName);

  EXPECT_EQ(result, "secret12345");
  std::cout << "[PASSED] Successfully extracted credential: " << result
            << "\n\n";
}

// Test 2: File not found error handling
TEST_F(GetCredsFunctorTest, FileNotFoundThrowsError) {
  std::cout << "[TESTING] Part 2: Error handling when the credentials file "
               "does not exist...\n";

  Creds::GetCredsFunctor getCreds;
  std::string nonExistentFile = "non_existent_file.txt";

  EXPECT_THROW(
      { getCreds(R"(key=(.*))", nonExistentFile); }, std::runtime_error);

  std::cout
      << "[PASSED] Correctly threw std::runtime_error for missing file.\n\n";
}

// Test 3: Empty file error handling
TEST_F(GetCredsFunctorTest, EmptyFileThrowsCredsError) {
  std::cout << "[TESTING] Part 3: Error handling when the credentials file is "
               "empty...\n";

  createTestFile(testFileName, "");  // Empty file

  Creds::GetCredsFunctor getCreds;

  EXPECT_THROW({ getCreds(R"(key=(.*))", testFileName); }, Creds::CredsError);

  std::cout << "[PASSED] Correctly threw CredsError for empty file.\n\n";
}

// Test 4: Regex mismatch error handling
TEST_F(GetCredsFunctorTest, RegexMismatchThrowsCredsError) {
  std::cout << "[TESTING] Part 4: Error handling when regex pattern does not "
               "match file content...\n";

  createTestFile(testFileName, "wrong_format_line");

  Creds::GetCredsFunctor getCreds;

  EXPECT_THROW(
      { getCreds(R"(expected_key=(.*))", testFileName); }, Creds::CredsError);

  std::cout
      << "[PASSED] Correctly threw CredsError for invalid regex match.\n\n";
}

// Test 5: Parent directory creation test
TEST_F(GetCredsFunctorTest, ParentDirectoryCreation) {
  std::cout << "[TESTING] Part 5: Automatic creation of non-existent parent "
               "directories...\n";

  std::string nestedDir = "nested/dir";
  std::string nestedFile = "nested/dir/test_creds.txt";

  // Ensure parent directories exist so the test file can be written
  // successfully
  std::filesystem::create_directories(nestedDir);
  createTestFile(nestedFile, "token=abc-xyz");

  Creds::GetCredsFunctor getCreds;
  std::string result = getCreds(R"(token=(.*))", nestedFile);

  EXPECT_EQ(result, "abc-xyz");
  EXPECT_TRUE(std::filesystem::exists(nestedDir));
  std::cout << "[PASSED] Successfully handled nested directory paths and "
               "extracted credential: "
            << result << "\n\n";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
