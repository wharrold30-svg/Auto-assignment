#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "Date.h"
#include "School.h"
#include "Teacher.h"
#include "Test.h"
#include "TimerManager.h"

class TimerManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code before each test if needed
  }

  void TearDown() override {
    // Cleanup code after each test if needed
  }
};

// ============================================================================
// 1. TEST: Basic Timer Scheduling & Expiration Trigger
// ============================================================================
TEST_F(TimerManagerTest, BasicExpirationTest) {
  std::cout << "\n--------------------------------------------------\n";
  std::cout
      << "[TEST PART] Testing: Basic Timer Scheduling & Expiration Trigger\n";

  std::atomic<bool> callback_fired{false};
  std::string received_test_id = "";

  // Omit unused 'test' parameter name to avoid compiler warnings
  auto mock_signal = [&](const std::string& test_id, AA::Test&) {
    received_test_id = test_id;
    callback_fired.store(true);
  };

  TimerManager manager([]() { return std::chrono::system_clock::now(); },
                       mock_signal);

  AA::School school("Oakridge High", "123 Education Lane", "MainGradebook2026");

  AA::Teacher teacher("Smith", "John", "T1", "john.smith@school.com", school);
  AA::Test test("Math Midterm", teacher, 100);

  // Set due date 1 second in the future
  auto t = std::time(nullptr) + 1;
  std::tm* tm_ptr = std::localtime(&t);
  DT::Date due_date;
  due_date.setTM(*tm_ptr);
  test.setDueDate(due_date);

  std::string expected_id = test.getTestID();
  manager.scheduleTest(test);

  // Wait for the timer thread / thread pool to process expiration
  int attempts = 0;
  while (!callback_fired.load() && attempts < 30) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attempts++;
  }

  EXPECT_TRUE(callback_fired.load())
      << "[FAILED] Basic Expiration: Callback failed to fire!";
  EXPECT_EQ(received_test_id, expected_id)
      << "[FAILED] Basic Expiration: Test ID mismatch!";

  std::cout << "[PASS] Basic Expiration Test passed successfully!\n";
  std::cout << "[STATUS] Test execution finished.\n";
  std::cout << "--------------------------------------------------\n";
}

// ============================================================================
// 2. TEST: Dynamic Reset with a Sooner Timer
// ============================================================================
TEST_F(TimerManagerTest, DynamicResetShorterTimerTest) {
  std::cout << "\n--------------------------------------------------\n";
  std::cout << "[TEST PART] Testing: Dynamic Reset when a shorter/sooner timer "
               "is injected\n";

  std::vector<std::string> triggered_order;
  std::mutex test_mtx;

  // Omit unused 'test' parameter name
  auto mock_signal = [&](const std::string& test_id, AA::Test&) {
    std::lock_guard<std::mutex> lock(test_mtx);
    triggered_order.push_back(test_id);
  };

  TimerManager manager([]() { return std::chrono::system_clock::now(); },
                       mock_signal);

  auto make_due_date = [](int seconds) {
    auto t = std::time(nullptr) + seconds;
    std::tm* tm_ptr = std::localtime(&t);
    DT::Date d;
    d.setTM(*tm_ptr);
    return d;
  };

  AA::School school("Oakridge High", "District 4", "MainGradebook2026");
  AA::Teacher teacher("Smith", "John", "T1", "john.smith@school.com", school);

  // 1. Schedule long test (e.g., 3 seconds out)
  AA::Test long_test("Final Exam", teacher, 100);
  long_test.setDueDate(make_due_date(3));
  manager.scheduleTest(long_test);

  // Let worker thread pick up the 3-second timer and enter cv.wait_until()
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // 2. Schedule shorter test (1 second out - should reset wait)
  AA::Test short_test("Pop Quiz", teacher, 50);
  short_test.setDueDate(make_due_date(1));
  manager.scheduleTest(short_test);

  // Polling loop to safely wait for both callbacks to complete execution via
  // thread pool
  int attempts = 0;
  while (attempts < 40) {
    {
      std::lock_guard<std::mutex> lock(test_mtx);
      if (triggered_order.size() >= 2) break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attempts++;
  }

  {
    std::lock_guard<std::mutex> lock(test_mtx);
    ASSERT_GE(triggered_order.size(), 2)
        << "[FAILED] Dynamic Reset: Did not trigger both events!";
    EXPECT_EQ(triggered_order[0], short_test.getTestID())
        << "[FAILED] Dynamic Reset: Shorter timer failed to trigger first!";
  }

  std::cout << "[PASS] Dynamic Reset Test passed successfully!\n";
  std::cout << "[STATUS] Test execution finished.\n";
  std::cout << "--------------------------------------------------\n";
}

// ============================================================================
// 3. TEST: Injected Mock Clock (Controlled Time Progression)
// ============================================================================
TEST_F(TimerManagerTest, MockClockControlTest) {
  std::cout << "\n--------------------------------------------------\n";
  std::cout << "[TEST PART] Testing: Injected Mock Clock for deterministic "
               "time control\n";

  auto mock_now = std::chrono::system_clock::now();
  std::atomic<bool> callback_fired{false};

  // Omit unused parameters
  auto mock_signal = [&](const std::string&, AA::Test&) {
    callback_fired.store(true);
  };

  TimerManager manager([&mock_now]() { return mock_now; }, mock_signal);

  AA::School school("Oakridge High", "District 4", "MainGradebook2026");
  AA::Teacher teacher("Smith", "John", "T1", "john.smith@school.com", school);
  AA::Test future_test("Project Due", teacher, 100);

  auto future_time = mock_now + std::chrono::hours(1);
  std::time_t ft_c = std::chrono::system_clock::to_time_t(future_time);
  std::tm* tm_ptr = std::localtime(&ft_c);
  DT::Date due_date;
  due_date.setTM(*tm_ptr);
  future_test.setDueDate(due_date);

  manager.scheduleTest(future_test);

  // Verify it hasn't fired prematurely
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_FALSE(callback_fired.load())
      << "[FAILED] Mock Clock: Timer fired prematurely!";

  // Fast-forward mock clock past the due time
  mock_now += std::chrono::hours(2);

  // Explicitly notify the manager so the timer thread wakes up and evaluates
  // the new mock time
  manager.notify();

  // Wait a brief moment for the worker thread and thread pool to process the
  // task
  int attempts = 0;
  while (!callback_fired.load() && attempts < 30) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attempts++;
  }

  EXPECT_TRUE(callback_fired.load()) << "[FAILED] Mock Clock: Timer failed to "
                                        "fire after mock clock was advanced!";

  std::cout << "[PASS] Mock Clock Control Test passed successfully! Time "
               "fast-forwarded deterministically.\n";
  std::cout << "[STATUS] Test execution finished.\n";
  std::cout << "--------------------------------------------------\n";
}
