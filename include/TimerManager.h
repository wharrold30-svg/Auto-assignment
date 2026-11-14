#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "Date.h"
#include "Report.h"
#include "Test.h"
// #include "gmail.h" // Uncomment if using Gmail tool headers

// ============================================================================
// LIGHTWEIGHT REUSABLE WORKER THREAD POOL (For Asynchronous Callbacks)
// ============================================================================
class CallbackThreadPool {
 private:
  std::vector<std::thread> m_workers;
  std::queue<std::function<void()>> m_tasks;
  std::mutex m_queueMutex;
  std::condition_variable m_cv;
  bool m_stop = false;

 public:
  explicit CallbackThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
      m_workers.emplace_back([this] {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
            if (m_stop && m_tasks.empty()) return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
          }
          // Execute the heavy task (grading, report building, emailing) off the
          // timer thread
          task();
        }
      });
    }
  }

  // Enqueue a new callback task into the pool
  template <class F>
  void enqueue(F&& f) {
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      if (m_stop) return;
      m_tasks.emplace(std::forward<F>(f));
    }
    m_cv.notify_one();
  }

  ~CallbackThreadPool() {
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_stop = true;
    }
    m_cv.notify_all();
    for (std::thread& worker : m_workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }
};

// ============================================================================
// TIMER EVENT STRUCTURE
// ============================================================================
struct TimerEvent {
  std::chrono::system_clock::time_point trigger_time;
  std::string test_id;

  bool operator>(const TimerEvent& other) const {
    return trigger_time > other.trigger_time;
  }
};

// ============================================================================
// TIMER MANAGER CLASS
// ============================================================================
class TimerManager {
 private:
  std::priority_queue<TimerEvent, std::vector<TimerEvent>,
                      std::greater<TimerEvent>>
      m_eventQueue;

  std::map<std::string, AA::Test> m_testStorage;
  std::mutex mtx;
  std::condition_variable cv;
  bool m_running = true;
  std::thread m_workerThread;

  // Reusable thread pool for offloading grading, reports, and email dispatches
  // Defaults to hardware concurrency (number of available CPU threads)
  CallbackThreadPool m_callbackPool;

  std::function<std::chrono::system_clock::time_point()> m_currentTimeFunc;
  std::function<void(const std::string& test_id, AA::Test& test)>
      m_signalCallback;

 public:
  TimerManager(
      std::function<std::chrono::system_clock::time_point()> clock_func =
          []() { return std::chrono::system_clock::now(); },
      std::function<void(const std::string&, AA::Test&)> on_signal =
          [](const std::string& id, AA::Test& test) {
            std::cout << ">>> [DUE DATE REACHED] Test ID: " << id << " ("
                      << test.getTestName()
                      << ") expired! Processing reports asynchronously...\n";

            // 1. Grade the tests automatically
            test.gradeTests();

            // 2. Extract Teacher information
            const auto& teacher = test.getTeacher();
            std::string teacher_email = teacher.getEmailAddr();

            // 3. Generate teacher report
            std::string teacher_report = AA::buildTeacherReport(test);
            std::string teacher_subject =
                "Test Report & Grades: " + test.getTestName();

            // Trigger Gmail send for the teacher (uncomment when ready)
            // if (!teacher_email.empty()) {
            //     gmail::send_message({}, {}, {}, teacher_subject,
            //     teacher_report, {teacher_email});
            // }

            // 4. Retrieve student grades and send individual reports
            auto student_grades_opt = test.getStudents();
            if (student_grades_opt.has_value()) {
              for (const auto& [student_ptr, grade] :
                   student_grades_opt.value()) {
                if (!student_ptr) continue;

                std::string student_email = student_ptr->getEmailAddr();
                std::string student_subject =
                    "Your Grade Report: " + test.getTestName();
                std::string student_report =
                    AA::buildStundentReport(student_ptr, test);

                // Trigger Gmail send for each student (uncomment when ready)
                // if (!student_email.empty()) {
                //     gmail::send_message({}, {}, {}, student_subject,
                //     student_report, {student_email});
                // }
              }
            }
          },
      size_t thread_pool_size = std::max(2u,
                                         std::thread::hardware_concurrency()))
      : m_callbackPool(thread_pool_size),
        m_currentTimeFunc(clock_func),
        m_signalCallback(on_signal) {
    m_workerThread = std::thread(&TimerManager::processQueue, this);
  }

  ~TimerManager() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      m_running = false;
    }
    cv.notify_one();
    if (m_workerThread.joinable()) {
      m_workerThread.join();
    }
    // CallbackThreadPool cleans up its worker pool automatically here
  }

  // Manually wake up worker thread (essential for mock clock acceleration &
  // resets)
  void notify() {
    std::lock_guard<std::mutex> lock(mtx);
    cv.notify_one();
  }

  // Add an AA::Test object directly into storage and queue its due date timer
  void scheduleTest(const AA::Test& test) {
    std::string test_id = test.getTestID();
    DT::Date due_date = test.getDueDate();
    auto target_time = due_date.toTimePoint();

    {
      std::lock_guard<std::mutex> lock(mtx);
      m_testStorage[test_id] = test;

      bool is_sooner =
          m_eventQueue.empty() || target_time < m_eventQueue.top().trigger_time;

      m_eventQueue.push({target_time, test_id});
      std::cout << "[Scheduled] Test \"" << test.getTestName()
                << "\" (ID: " << test_id << ") added.\n";

      if (is_sooner) {
        std::cout << "-> [RESET] New soonest test due date detected! Resetting "
                     "active wait...\n";
        cv.notify_one();
      }
    }
  }

  // Optional: Retrieve a test from storage by its ID
  std::optional<AA::Test> getTest(const std::string& test_id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = m_testStorage.find(test_id);
    if (it != m_testStorage.end()) {
      return it->second;
    }
    return std::nullopt;
  }

 private:
  void processQueue() {
    while (true) {
      std::unique_lock<std::mutex> lock(mtx);

      if (!m_running && m_eventQueue.empty()) break;

      if (m_eventQueue.empty()) {
        cv.wait(lock, [this] { return !m_eventQueue.empty() || !m_running; });
        if (!m_running && m_eventQueue.empty()) break;
      }

      auto next_event = m_eventQueue.top();
      auto now = m_currentTimeFunc();

      if (now >= next_event.trigger_time) {
        // Due date reached! Pop it from queue immediately
        m_eventQueue.pop();

        auto test_it = m_testStorage.find(next_event.test_id);
        if (test_it != m_testStorage.end()) {
          AA::Test triggered_test = test_it->second;
          std::string tid = next_event.test_id;

          // Unlock the timer queue mutex before dispatching work
          lock.unlock();

          // SCALABILITY WIN: Hand the heavy lifting (grading, reports, emails)
          // off to the reusable thread pool instead of blocking or spawning raw
          // threads.
          m_callbackPool.enqueue([this, tid, triggered_test]() mutable {
            m_signalCallback(tid, triggered_test);
          });
        }
      } else {
        cv.wait_until(lock, next_event.trigger_time);
      }
    }
  }
};

#endif  // TIMER_MANAGER_H
