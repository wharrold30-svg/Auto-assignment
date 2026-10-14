#include "Date.h"

#include <gtest/gtest.h>

#include <ctime>
#include <sstream>
#include <string>

#include "nlohmann/json.hpp"

namespace DT {

// Helper function to create a populated std::tm struct
std::tm createTestTM(int year = 124, int month = 0, int day = 15, int hour = 12,
                     int min = 0, int sec = 0) {
  std::tm time_struct{};
  time_struct.tm_year = year;  // Years since 1900 (124 = 2024)
  time_struct.tm_mon = month;  // 0-indexed (0 = Jan)
  time_struct.tm_mday = day;
  time_struct.tm_hour = hour;
  time_struct.tm_min = min;
  time_struct.tm_sec = sec;
  time_struct.tm_isdst = -1;  // Let mktime determine DST
  return time_struct;
}

// ==========================================
// Test Suite: Constructors & Basic State
// ==========================================
TEST(DateTest, DefaultConstructor) {
  Date date;
  // Checks default state behavior
  EXPECT_FALSE(date.isLT());
}

TEST(DateTest, ParameterizedTMConstructor) {
  std::tm input_tm = createTestTM();
  Date date(input_tm);

  std::tm result_tm = date.getTM();
  EXPECT_EQ(result_tm.tm_year, input_tm.tm_year);
  EXPECT_EQ(result_tm.tm_mon, input_tm.tm_mon);
  EXPECT_EQ(result_tm.tm_mday, input_tm.tm_mday);
}

TEST(DateTest, CopyConstructorAndAssignment) {
  std::tm input_tm = createTestTM();
  Date original(input_tm);

  Date copy_constructed(original);
  EXPECT_EQ(copy_constructed.getTM().tm_mday, original.getTM().tm_mday);

  Date copy_assigned;
  copy_assigned = original;
  EXPECT_EQ(copy_assigned.getTM().tm_mday, original.getTM().tm_mday);
}

// ==========================================
// Test Suite: Utility & Manipulation Methods
// ==========================================
TEST(DateTest, Swap) {
  std::tm tm1 = createTestTM(124, 0, 1);
  std::tm tm2 = createTestTM(124, 5, 20);

  Date date1(tm1);
  Date date2(tm2);

  date1.swap(date2);

  EXPECT_EQ(date1.getTM().tm_mon, 5);
  EXPECT_EQ(date2.getTM().tm_mon, 0);
}

TEST(DateTest, ClearAndEmpty) {
  std::tm input_tm = createTestTM();
  Date date(input_tm);

  date.clear();
  EXPECT_TRUE(date.empty());
}

// ==========================================
// Test Suite: Operators (Comparison & Subtraction)
// ==========================================
TEST(DateTest, SubtractionOperator) {
  std::tm earlier_tm = createTestTM(124, 0, 1, 12, 0, 0);
  std::tm later_tm = createTestTM(124, 0, 1, 12, 1, 0);  // 60 seconds later

  Date earlier(earlier_tm);
  Date later(later_tm);

  // Tests time difference calculation in seconds
  EXPECT_DOUBLE_EQ(later - earlier, -60.0);
  EXPECT_DOUBLE_EQ(earlier - later, 60.0);
}

TEST(DateTest, EqualityAndSpaceshipOperators) {
  std::tm tm1 = createTestTM(124, 0, 10);
  std::tm tm2 = createTestTM(124, 0, 10);
  std::tm tm_newer = createTestTM(124, 0, 15);

  Date date1(tm1);
  Date date2(tm2);
  Date date_newer(tm_newer);

  EXPECT_TRUE(date1 == date2);
  EXPECT_FALSE(date1 == date_newer);

  // According to Date.h comments: newer dates are less than older dates
  EXPECT_TRUE(date_newer < date1);
  EXPECT_TRUE(date1 > date_newer);
}

// ==========================================
// Test Suite: Serialization & Streams
// ==========================================
TEST(DateTest, JsonSerialization) {
  std::tm input_tm = createTestTM(124, 2, 15, 10, 30, 45);
  Date original_date(input_tm);

  nlohmann::json j = original_date;
  Date deserialized_date = j.get<Date>();

  EXPECT_EQ(original_date.getTM().tm_year, deserialized_date.getTM().tm_year);
  EXPECT_EQ(original_date.getTM().tm_mon, deserialized_date.getTM().tm_mon);
  EXPECT_EQ(original_date.getTM().tm_mday, deserialized_date.getTM().tm_mday);
}

TEST(DateTest, OutputStreamOperator) {
  std::tm input_tm = createTestTM();
  Date date(input_tm);

  std::ostringstream os;
  os << date;

  EXPECT_FALSE(os.str().empty());
}

// ==========================================
// Test Suite: Custom Exceptions
// ==========================================
TEST(DateTest, DateErrorFormatting) {
  try {
    throw DateError("Invalid input format");
  } catch (const DateError& e) {
    EXPECT_STREQ(e.what(), "Date error: Invalid input format");
  }
}

}  // namespace DT

// Main entry point for Google Test
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
