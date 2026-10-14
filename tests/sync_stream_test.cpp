#include "Sync_stream.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ============================================================================
// PART 1: Testing Sync::Transaction
// ============================================================================

TEST(TransactionTest, DefaultConstructorIsEmpty) {
  // PART TESTED: Transaction default constructor creates an empty buffer
  Sync::Transaction t;
  EXPECT_EQ(t.str(), "");
}

TEST(TransactionTest, StringViewConstructor) {
  // PART TESTED: Transaction constructor with initial string_view
  Sync::Transaction t("Hello");
  EXPECT_EQ(t.str(), "Hello");
}

TEST(TransactionTest, InsertionOperatorChaining) {
  // PART TESTED: Transaction operator<< for various data types and method
  // chaining
  Sync::Transaction t("Prefix: ");
  t << "Count = " << 42 << " | Status = " << true;

  EXPECT_EQ(t.str(), "Prefix: Count = 42 | Status = 1");
}

TEST(TransactionTest, StreamOutputOperator) {
  // PART TESTED: Sync::operator<<(std::ostream&, Sync::Transaction const&)
  Sync::Transaction t("Stream output test");
  std::ostringstream oss;
  oss << t;

  EXPECT_EQ(oss.str(), "Stream output test");
}

// ============================================================================
// PART 2: Testing Sync::SyncStream
// ============================================================================

TEST(SyncStreamTest, BasicOutputStreaming) {
  // PART TESTED: SyncStream single-threaded operator<< output formatting
  std::ostringstream oss;
  Sync::SyncStream stream(oss);

  stream << "Test " << 123 << " " << 45.67;

  EXPECT_EQ(oss.str(), "Test 123 45.67");
}

TEST(SyncStreamTest, CustomTypeStreaming) {
  // PART TESTED: SyncStream operator<< handling custom types via Transaction
  std::ostringstream oss;
  Sync::SyncStream stream(oss);

  Sync::Transaction t("Transaction Data");
  stream << t;

  EXPECT_EQ(oss.str(), "Transaction Data");
}

TEST(SyncStreamTest, ThreadSafetyAndConcurrency) {
  // PART TESTED: SyncStream m_mutex protection under concurrent thread writes
  std::ostringstream oss;
  Sync::SyncStream stream(oss);

  constexpr int num_threads = 10;
  constexpr int writes_per_thread = 50;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&stream, i]() {
      for (int j = 0; j < writes_per_thread; ++j) {
        // Build the string inside a Transaction first so message text stays
        // intact
        Sync::Transaction t;
        t << "[Thread " << i << " Write " << j << "]\n";
        stream << t;
      }
    });
  }

  for (auto& th : threads) {
    th.join();
  }

  // Verify total expected output length to ensure no lost/corrupted writes
  std::string result = oss.str();
  EXPECT_FALSE(result.empty());

  // Count total occurrences of newline to ensure all writes completed
  size_t line_count = 0;
  for (char c : result) {
    if (c == '\n') line_count++;
  }

  EXPECT_EQ(line_count, static_cast<size_t>(num_threads * writes_per_thread));
}

// Main entry point for running Google Test
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
