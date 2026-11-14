![CI Build Status](https://github.com)


<!-- Usage -->
Because your implementation sets the default factory lambda to instantiate RealConnection("localhost", 8080) automatically, your production code continues to work out of the box without any extra setup:


// Production code just calls the singleton methods normally
AA::OpenTestSingleton::Instance().add(myTest);
// Under the hood, sendDueDate() uses the default RealConnection factory automatically.


but if you ever need to change your production server host or port dynamically (e.g., pulling from an environment variable or config file), you can set the factory once at the very beginning of your application's main() function:

// In your main.cpp (Production Entry Point)
#include "OpenTestSingleton.h"
#include "RealConnection.h"
#include <cstdlib> // for getenv

int main(int argc, char* argv[]) {
    // Optional: Configure production host/port from environment or config
    std::string host = "127.0.0.1";
    int port = 8080;

    if (const char* env_p = std::getenv("TEST_SERVER_PORT")) {
        port = std::stoi(env_p);
    }

    // Bind the factory once at startup with your custom production settings
    AA::OpenTestSingleton::SetConnectionFactory([host, port](const std::string&, int) {
        return std::make_unique<RealConnection>(host, port);
    });

    // ... start the rest of your application ...
    return 0;
}


//////////////////////////////
Using TimeManager:

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

#include "TimerManager.h"
#include "Test.h"
#include "Teacher.h"
#include "School.h"
#include "Student.h"
#include "Date.h"

int main() {
    std::cout << "=== Starting Multi-Threaded Test & Notification Server ===\n";

    // 1. Initialize TimerManager with a background thread pool (e.g., 4 worker threads)
    TimerManager timerManager([]() { return std::chrono::system_clock::now(); },
        [](const std::string& id, AA::Test& test) {
            std::cout << "[Callback] Processing expired test: " << id
                      << " (" << test.getTestName() << ")\n";
            test.gradeTests();
            // Automatically triggers report generation & email dispatches via thread pool
        },
        4 // Thread pool size
    );

    AA::School school("Oakridge High", "123 Education Lane", "Gradebook_DB_01");
    AA::Teacher teacherSmith("Smith", "Jane", "T-101", "smith@school.com", school);
    AA::Teacher teacherDoe("Doe", "John", "T-102", "doe@school.com", school);

    auto make_due_date = [](int seconds) {
        auto t = std::time(nullptr) + seconds;
        std::tm* tm_ptr = std::localtime(&t);
        DT::Date d;
        d.setTM(*tm_ptr);
        return d;
    };

    // 2. Simulate high-throughput multi-threaded client requests (Concurrent Schedulers)
    // Multiple threads calling scheduleTest() simultaneously against the shared TimerManager
    std::vector<std::thread> schedulerThreads;

    schedulerThreads.emplace_back([&timerManager, &teacherSmith, &make_due_date]() {
        AA::Test quiz("Physics Quiz", teacherSmith, 50);
        quiz.setDueDate(make_due_date(2)); // Expires in 2 seconds
        timerManager.scheduleTest(quiz);
    });

    schedulerThreads.emplace_back([&timerManager, &teacherDoe, &make_due_date]() {
        AA::Test midterm("Algebra Midterm", teacherDoe, 100);
        midterm.setDueDate(make_due_date(4)); // Expires in 4 seconds
        timerManager.scheduleTest(midterm);
    });

    schedulerThreads.emplace_back([&timerManager, &teacherSmith, &make_due_date]() {
        AA::Test finalExam("Chemistry Final", teacherSmith, 100);
        finalExam.setDueDate(make_due_date(6)); // Expires in 6 seconds
        timerManager.scheduleTest(finalExam);
    });

    // Join the scheduler threads
    for (auto& t : schedulerThreads) {
        if (t.joinable()) t.join();
    }

    std::cout << "[Main] All tests scheduled concurrently. Waiting for background execution...\n";

    // Let the server run long enough for all timers to expire and process asynchronously
    std::this_thread::sleep_for(std::chrono::seconds(7));

    std::cout << "=== Shutting Down Server Gracefully ===\n";
    return 0;
}

