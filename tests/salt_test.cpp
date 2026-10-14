#include "Salt.h"  // Ensure salt.h is in your include path

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string>

namespace Salt {
namespace {

class SaltTest : public ::testing::Test {
 protected:
  // Character set defined in the Salt class (excluding null terminator)
  const std::string allowed_chars =
      "0123456789"
      "!@#$%&^"
      " "
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
};

// Tests that the generated salt matches the requested length
TEST_F(SaltTest, GeneratesCorrectLength) {
  std::size_t expected_length = 32;
  Salt salter(expected_length);

  std::string generated_salt = salter();

  EXPECT_EQ(generated_salt.length(), expected_length);
}

// Tests edge case where requested length is zero
TEST_F(SaltTest, GeneratesEmptyStringForZeroLength) {
  Salt salter(0);

  std::string generated_salt = salter();

  EXPECT_TRUE(generated_salt.empty());
  EXPECT_EQ(generated_salt.length(), 0);
}

// Tests that generated salts consist only of expected characters
TEST_F(SaltTest, ContainsOnlyAllowedCharacters) {
  Salt salter(100);
  std::string generated_salt = salter();

  for (char c : generated_salt) {
    EXPECT_NE(allowed_chars.find(c), std::string::npos)
        << "Unexpected character found in salt: '" << c
        << "' (ASCII: " << static_cast<int>(c) << ")";
  }
}

// Tests that the salt does not contain null characters ('\0')
TEST_F(SaltTest, DoesNotContainNullTerminator) {
  Salt salter(200);
  std::string generated_salt = salter();

  EXPECT_EQ(generated_salt.find('\0'), std::string::npos)
      << "Salt contains an embedded null character, indicating off-by-one "
         "distribution bounds.";
}

// Tests that sequential calls produce distinct random output
TEST_F(SaltTest, GeneratesUniqueSalts) {
  Salt salter(16);
  constexpr int iterations = 100;
  std::set<std::string> unique_salts;

  for (int i = 0; i < iterations; ++i) {
    unique_salts.insert(salter());
  }

  // Expect all generated salts to be unique
  EXPECT_EQ(unique_salts.size(), iterations);
}

// Tests multiple instances of the Salt class to ensure independent operation
TEST_F(SaltTest, MultipleInstancesFunctionIndependently) {
  Salt salter1(16);
  Salt salter2(64);

  EXPECT_EQ(salter1().length(), 16);
  EXPECT_EQ(salter2().length(), 64);
}

}  // namespace
}  // namespace Salt

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
