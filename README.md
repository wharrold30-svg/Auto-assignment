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


//////////////////////////////////////////////// 
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

////////////////////////////////////////////////

// 1. Create student bound to your connection
Student student(m_conn);
student.save(); // Uses stored m_conn automatically!

// 2. Read an existing student by string ID (static method still takes connection)
Student fetchedStudent(m_conn);
Student::read(m_conn, "student_id", "STU123", fetchedStudent);

// 3. Update the student (uses stored m_conn automatically!)
fetchedStudent.update("student_id", "STU123");

// 4. Remove a Test Question by integer ID
TestQuestion::remove(m_conn, "id", 42);

////////////////////////////////////////////////

## SQL Bridge Objects (`AA::Data`)

All bridge classes inherit from `AA::Data::DataBase<Bridge, Derived, IdType>` (defaulting `IdType` to `std::string`) and are non-copyable / non-movable (`delete` copy/move constructors and assignment operators).

### Method Reference

| Class | Method | Purpose |
| :--- | :--- | :--- |
| `DataBase` | `getTableName()` | Returns the target SQL table name |
| `DataBase` | `save(model)` | Inserts a model via `toSSQLS()` |
| `DataBase` | `update(model, idColumn, idValue)` | Updates a row matching `idColumn = idValue` |
| `DataBase` | `read(idColumn, idValue, out_model)` | Loads a single row into `out_model` |
| `DataBase` | `remove(idColumn, idValue)` | Deletes rows matching `idColumn = idValue` |
| `gradebook` | `loadAllGradeBooks(out_gradeBooks)` | Loads all `(name, Gradebook)` pairs |
| `gradebook` | `loadGradeBookForSchool(schoolName, out_gradeBook)` | Loads the gradebook assigned to a school |
| `gradebook` | `updateGradeBookForSchool(schoolName, gradeBook)` | Updates a school's gradebook |
| `school` | `transferStudentSchool(studentId, newSchoolName)` | Moves a student to a new school |
| `school` | `removeStudentFromSchoolCompletely(studentId, schoolName)` | Removes a student from a matching school and related records |
| `school` | `transferTeacherSchool(teacherId, newSchoolName)` | Moves a teacher to a new school |
| `school` | `removeTeacherFromSchool(teacherId)` | Detaches a teacher from their school |
| `school` | `loadTeachersForSchool(schoolName, out_teachers)` | Loads teachers belonging to a school |
| `school` | `loadStudentsForSchool(schoolName, out_students)` | Loads students belonging to a school |
| `student` | `loadStudentSchedule(studentId, out_schedule)` | Loads `(teacherId, classPeriod)` tuples for a student |
| `student` | `loadOpenTestsForStudent(studentId, out_tests)` | Loads open tests assigned to a student |
| `student` | `getStudentAssignments(studentId, out_assignments)` | Loads `GradeRecord` entries for a student |
| `student` | `returnStudentTest(...)` | Records a student's test submission, score/grade, and status |
| `student` | `loadStudentByEmail(email, out_student)` | Finds a student by email address |
| `teacher` | `loadCredentials(teacherId, out_salt, out_password)` | Loads a teacher's salt and password hash |
| `teacher` | `saveCredentials(teacherId, salt, password)` | Saves a teacher's salt and password hash |
| `teacher` | `loadTestsForTeacher(teacherId, out_tests)` | Loads tests authored by a teacher |
| `teacher` | `assignOpenTestToStudent(studentId, openTestId)` | Assigns an open test to one student |
| `teacher` | `assignOpenTestToMultipleStudents(studentIds, openTestId)` | Assigns an open test to multiple students |
| `teacher` | `loadOpenTestsForTeacher(teacherId, out_tests)` | Loads open tests created by a teacher |
| `teacher` | `loadTeacherRoster(teacherId, out_roster)` | Loads `(studentId, classPeriod)` pairs for a teacher |
| `teacher` | `saveTeacherRoster(teacherId, studentPeriodPairs)` | Replaces/saves a teacher's roster pairs |
| `teacher` | `enrollStudentInClass(teacherId, studentId, classPeriod)` | Enrolls a student in a teacher's class period |
| `teacher` | `removeStudentFromClass(teacherId, studentId, classPeriod)` | Removes a student from a teacher's class period |
| `teacher` | `loadTeacherByEmail(email, out_teacher)` | Finds a teacher by email address |
| `teacher` | `loadRosterForPeriod(teacherId, classPeriod, out_studentIds)` | Loads student IDs for a teacher and specific period |
| `test` | `saveOpenTest(openTest)` | Saves an `AA::OpenTest` record |
| `test` | `loadOpenTestsByDateRange(startDate, endDate, out_tests)` | Loads open tests within a date range |
| `test` | `loadTestsPaginated(limit, offset, out_tests)` | Loads a slice of tests with pagination |
| `test` | `saveTestQuestions(testId, questions)` | Saves question/answer items (`TestVec`) for a test |
| `test` | `saveTestWithQuestions(test)` | Saves a test along with its questions |
| `test` | `loadTestQuestions(testId, out_questions)` | Loads questions for a test |
| `test` | `removeTestQuestions(testId)` | Removes questions for a test |
| `test` | `loadTestWithQuestions(testId, out_test)` | Loads a test hydrated with its questions |

---

### Usage Examples

### Base CRUD Operations (`AA::Data::DataBase`)

Every bridge exposes `getTableName()`, `save()`, `update()`, `read()`, and `remove()` via `DataBase`:

```cpp
#include <mysql++/mysql++.h>
#include <iostream>
#include "Gradebook.h"
#include "sql/gradebook.h"

void baseCrudExample(mysqlpp::Connection& conn) {
  AA::Data::gradebook gbBridge(conn);
  std::cout << "Table: " << gbBridge.getTableName() << '\n';  // "gradebooks"

  AA::Gradebook gb;
  gb.setName("Canvas");
  gb.setURL("[https://canvas.example.edu](https://canvas.example.edu)");
  gb.setPostTemp("post_canvas.py");

  // INSERT via model.toSSQLS()
  gbBridge.save(gb);

  // UPDATE via model.toSSQLS() where idColumn == idValue
  gb.setURL("[https://updated.canvas.example.edu](https://updated.canvas.example.edu)");
  gbBridge.update(gb, "name", "Canvas");

  // READ into out_model via fromSSQLS()
  AA::Gradebook loaded;
  if (gbBridge.read("name", "Canvas", loaded)) {
    std::cout << loaded.getURL() << '\n';
  }

  // DELETE where idColumn == idValue
  gbBridge.remove("name", "Canvas");
}
////////////////////////////////////////////////

#include <mysql++/mysql++.h>
#include <utility>
#include <vector>
#include "Gradebook.h"
#include "sql/gradebook.h"

void gradebookBridgeExample(mysqlpp::Connection& conn) {
  AA::Data::gradebook gbBridge(conn);

  AA::Gradebook gb;
  gb.setName("Canvas");
  gb.setURL("[https://canvas.example.edu](https://canvas.example.edu)");
  gb.setPostTemp("post_canvas.py");

  // Update the gradebook associated with a school
  gbBridge.updateGradeBookForSchool("North High", gb);

  // Load the gradebook for a specific school
  AA::Gradebook schoolGb;
  gbBridge.loadGradeBookForSchool("North High", schoolGb);

  // Load all (name, Gradebook) pairs
  std::vector<std::pair<std::string, AA::Gradebook>> allGradebooks;
  gbBridge.loadAllGradeBooks(allGradebooks);
}

////////////////////////////////////////////////

#include <mysql++/mysql++.h>
#include <vector>
#include "Student.h"
#include "Teacher.h"
#include "sql/school.h"

void schoolBridgeExample(mysqlpp::Connection& conn) {
  AA::Data::school schoolBridge(conn);

  // Load all teachers or students at a school
  std::vector<AA::Teacher> teachers;
  schoolBridge.loadTeachersForSchool("North High", teachers);

  std::vector<AA::Student> students;
  schoolBridge.loadStudentsForSchool("North High", students);

  // Transfer a teacher or student to another school
  schoolBridge.transferTeacherSchool("T100", "South High");
  schoolBridge.transferStudentSchool("S100", "South High");

  // Remove a teacher from their school
  schoolBridge.removeTeacherFromSchool("T100");

  // Remove a student completely only if currently in schoolName
  schoolBridge.removeStudentFromSchoolCompletely("S100", "South High");
}

////////////////////////////////////////////////

#include <mysql++/mysql++.h>
#include <string>
#include <tuple>
#include <vector>
#include "Date.h"
#include "GradeRecord.h"
#include "OpenTest.h"
#include "Student.h"
#include "sql/student.h"

void studentBridgeExample(mysqlpp::Connection& conn) {
  AA::Data::student studentBridge(conn);

  // Look up a student by email address
  AA::Student found;
  studentBridge.loadStudentByEmail("aturing@north.edu", found);

  // Load (teacherId, classPeriod) tuples for a student
  std::vector<std::tuple<std::string, unsigned int>> schedule;
  studentBridge.loadStudentSchedule("S100", schedule);

  // Load open tests assigned to the student
  std::vector<AA::OpenTest> openTests;
  studentBridge.loadOpenTestsForStudent("S100", openTests);

  // Submit/return a student test (defaults: returnTest="", wrongAnswers="", grade=0.0f)
  const DT::Date submittedAt("2026-11-18 14:30:00");
  studentBridge.returnStudentTest("S100", 42, submittedAt, 90.0, "Submitted",
                                  R"(["4"])", R"([])", 90.0f);

  // Load grade records / assignments for the student
  std::vector<AA::GradeRecord> assignments;
  studentBridge.getStudentAssignments("S100", assignments);
}

////////////////////////////////////////////////

#include <mysql++/mysql++.h>
#include <string>
#include <utility>
#include <vector>
#include "OpenTest.h"
#include "Teacher.h"
#include "Test.h"
#include "sql/teacher.h"

void teacherBridgeExample(mysqlpp::Connection& conn) {
  AA::Data::teacher teacherBridge(conn);

  // Save and load teacher credentials (salt + password hash)
  teacherBridge.saveCredentials("T100", "random_salt", "hashed_password");
  std::string salt;
  std::string password;
  teacherBridge.loadCredentials("T100", salt, password);

  // Find a teacher by email
  AA::Teacher found;
  teacherBridge.loadTeacherByEmail("ghopper@north.edu", found);

  // Enroll, remove, save, or load roster entries ((studentId, classPeriod))
  teacherBridge.enrollStudentInClass("T100", "S100", 2);
  teacherBridge.removeStudentFromClass("T100", "S100", 2);

  const std::vector<std::pair<std::string, unsigned int>> rosterPairs = {
      {"S100", 2}, {"S101", 2}};
  teacherBridge.saveTeacherRoster("T100", rosterPairs);

  std::vector<std::pair<std::string, unsigned int>> loadedRoster;
  teacherBridge.loadTeacherRoster("T100", loadedRoster);

  std::vector<std::string> period2Students;
  teacherBridge.loadRosterForPeriod("T100", 2, period2Students);

  // Load tests and open tests belonging to the teacher
  std::vector<AA::Test> tests;
  teacherBridge.loadTestsForTeacher("T100", tests);

  std::vector<AA::OpenTest> openTests;
  teacherBridge.loadOpenTestsForTeacher("T100", openTests);

  // Assign an open test (by int ID) to one or multiple students
  teacherBridge.assignOpenTestToStudent("S100", 42);
  teacherBridge.assignOpenTestToMultipleStudents({"S100", "S101"}, 42);
}

////////////////////////////////////////////////

#include <mysql++/mysql++.h>
#include <string>
#include <vector>
#include "Date.h"
#include "OpenTest.h"
#include "Test.h"
#include "sql/test.h"

void testBridgeExample(mysqlpp::Connection& conn) {
  AA::Data::test testBridge(conn);

  AA::Test quiz;
  quiz.setTestID("TEST_01");
  quiz.setTestName("Logic Quiz");

  AA::QandA q1;
  q1.setQuestion("2 + 2?");
  q1.setAnswer("4");
  TestVec questions = {q1};

  // Save questions or a full test with its questions
  testBridge.saveTestQuestions("TEST_01", questions);
  testBridge.saveTestWithQuestions(quiz);

  // Load questions or a hydrated AA::Test with questions
  TestVec loadedQuestions;
  testBridge.loadTestQuestions("TEST_01", loadedQuestions);

  AA::Test loadedQuiz;
  testBridge.loadTestWithQuestions("TEST_01", loadedQuiz);

  // Paginated test loading
  std::vector<AA::Test> page;
  testBridge.loadTestsPaginated(/*limit=*/10, /*offset=*/0, page);

  // Save an open test and query open tests in a date range
  AA::OpenTest ot;
  ot.setID(42);
  ot.setTestID("TEST_01");
  ot.setTeacherID("T100");
  ot.setCreationDate(DT::Date("2026-11-18 09:00:00"));
  ot.setDueDate(DT::Date("2026-11-25 23:59:00"));
  ot.setDuration(60);
  ot.setStatus("Open");
  testBridge.saveOpenTest(ot);

  std::vector<AA::OpenTest> rangedOpenTests;
  testBridge.loadOpenTestsByDateRange(DT::Date("2026-11-01 00:00:00"),
                                      DT::Date("2026-11-30 23:59:00"),
                                      rangedOpenTests);

  // Remove questions for a test
  testBridge.removeTestQuestions("TEST_01");
}

///////////////////////////////////////////////

## Server Setup & Request Pipeline (`main.cpp`)

Requests flow from the socket event loop into domain handlers and back out as JSON responses through five layers:

1. **`ServerFunctor`** — Runs the `select()` socket loop, accepts connections, reads incoming frames via `Connection::rx()`, wraps each message in a `Connection::ConnectContainer(msg, conn)`, dispatches `m_process(container)`, and flushes replies via `Connection::sx()` when `writeNeeded` is `true`.
2. **`ContainerBridge`** — Adapts `RequestProcessor` to `std::function<void(Connection::ConnectContainer&)>`, reading `container.getBuffer()` and replacing it with the serialized JSON response via `container.setBuffer(reply)`.
3. **`RequestProcessor`** — Parses raw JSON (`nlohmann::json::parse`), catches parse/runtime errors into structured JSON error payloads, invokes `HandlerFactory::newHandler(std::move(req), pool)` and `handler->execute()`, and returns `response.dump()`.
4. **`HandlerFactory::newHandler`** — Validates `"data"` and `"action"` on the request object and constructs the matching concrete handler (`TestHandler`, `TeacherHandler`, `StudentHandler`, `SchoolHandler`, `GradebookHandler`).
5. **`Handler::execute()` & `ConnectionPool`** — Validates payload/action fields, acquires a connection from `AA::Data::ConnectionPool` for the execution scope, runs `executeImpl(...)` against the database repositories, and returns a JSON status/result or error object.

### Example `main.cpp` Wiring

```cpp
#include <mysql++/mysql++.h>

#include <csignal>

#include "ContainerBridge.h"
#include "ServerFunctor.h"
#include "sql/ConnectionPool.h"

int main() {
  // Prevent broken client sockets from raising SIGPIPE and terminating the process.
  std::signal(SIGPIPE, SIG_IGN);

  // Initialize MySQL++ per-thread state before creating/using pooled connections.
  mysqlpp::Connection::thread_start();

  AA::Data::ConnectionPool pool{/* ... credentials / pool arguments ... */};
  AA::Server::ContainerBridge bridge{pool};

  AA::ServerFunctor server(
      /*port=*/8080,
      /*serverName=*/"AutoAssignmentServer",
      /*process=*/bridge.asCallback(),
      /*writeNeeded=*/true);

  server();

  mysqlpp::Connection::thread_end();
  return 0;
}
```

### Key Checklist When Wiring or Extending

* **Keep `writeNeeded = true` with `ContainerBridge`:** `ContainerBridge` stores the JSON response back into `ConnectContainer` via `setBuffer(reply)` so `ServerFunctor` queues and sends it over `conn->sx(...)`.
* **Match `Handler` and `HandlerFactory` constructors:** Concrete handlers inherit `Handler(nlohmann::json request, AA::Data::ConnectionPool& pool)` via `using Handler::Handler;`.
* **MySQL++ thread lifecycle:** Call `mysqlpp::Connection::thread_start()` when a thread starts using `ConnectionPool` and `mysqlpp::Connection::thread_end()` before that thread exits.

---

## JSON Wire Contract (Requests & Responses)

Incoming messages read by `Connection::rx()` must be JSON objects containing `"data"` (the target entity) and `"action"` (the operation name), with an optional `"payload"` object for action parameters.

* **`data`** (`string`, required): Entity routed by `HandlerFactory::newHandler` — `"student"`, `"teacher"`, `"test"`, `"school"`, or `"gradebook"`.
* **`action`** (`string`, required): Operation dispatched in `Handler::executeImpl` (e.g., `"create"`, `"get"`, `"update"`, `"delete"`, `"getAll"`).
* **`payload`** (`object`, optional): Action-specific fields (defaults to an empty object `{}` if omitted or `null`).

### Example 1: Creating a `teacher`

**Request:**

```json
{
  "data": "teacher",
  "action": "create",
  "payload": {
    "firstName": "Grace",
    "lastName": "Hopper",
    "email": "ghopper@school.edu",
    "department": "Computer Science",
    "schoolId": 101
  }
}
```

**Success Response:**

```json
{
  "status": "ok",
  "data": "teacher",
  "action": "create",
  "result": {
    "id": 7,
    "firstName": "Grace",
    "lastName": "Hopper",
    "email": "ghopper@school.edu",
    "department": "Computer Science",
    "schoolId": 101
  }
}
```

### Example 2: Fetching a `student`

**Request:**

```json
{
  "data": "student",
  "action": "get",
  "payload": {
    "id": 42
  }
}
```

**Success Response:**

```json
{
  "status": "ok",
  "data": "student",
  "action": "get",
  "result": {
    "id": 42,
    "firstName": "Ada",
    "lastName": "Lovelace",
    "email": "alovelace@school.edu",
    "gradeLevel": 11,
    "schoolId": 101
  }
}
```

### Example 3: Error Response (`student` Missing Required Field)

Malformed JSON, invalid routing fields, missing payload keys, or database/runtime exceptions are caught at the `RequestProcessor` or `Handler::execute()` boundary and returned as a structured JSON error:

```json
{
  "status": "error",
  "data": "student",
  "action": "get",
  "message": "Missing required field: id"
}
```
///////////////////////////////////////////////

Server Concurrency, Scaling Roadmap, and epoll Transition
​Single-Threaded Event Loop vs. Thread-Per-Connection
​A single-threaded I/O multiplexing loop (such as select(), poll(), or epoll) can outperform traditional thread-per-connection servers when handling many concurrent connections with fast, non-blocking request handling:
​Zero Context-Switching Overhead: With a thread per client, the OS scheduler pauses and resumes threads, flushing CPU caches and registers, whereas a single-threaded loop stays on one core and avoids kernel context switches between user threads.
​No Lock Contention: Because m_connectVec, m_processQueue, and pendingWrites are touched by only one thread in ServerFunctor::operator(), there is no mutex contention or cache-line bouncing across CPU cores.
​Lower Memory Footprint: Each OS thread reserves its own stack (often 1–8 MB on Linux), whereas one event loop multiplexes many sockets with minimal memory overhead.
​High Cache Locality: Keeping socket state, request buffers, and response queues in a tight loop keeps hot data in CPU cache.
​The Trade-Off: This speed advantage holds while m_process(current) finishes quickly. Because ServerFunctor runs m_process(current) synchronously in its loop, slow or blocking operations inside a handler (such as heavy database queries, disk I/O, or outbound SMTP calls via libcurl) delay subsequent select() iterations for other connected clients.
​Scaling Roadmap: Starting Small to Thousands of Concurrent Users
​Starting small and scaling incrementally keeps early development simple while preserving a clean path toward higher concurrency:
​1. Stage 1 — Early Users (Tens to Low Hundreds): Keep the single-threaded event loop and ConnectionPool as they are, focusing on correctness, clean error handling, and fast SQL queries.
​2. Stage 2 — Growing Traffic (Hundreds to ~1,000+ Concurrent Connections): When select() limits become noticeable (such as FD_SETSIZE or scanning sockets every tick), replace select() inside ServerFunctor with epoll on Linux while keeping ConnectContainer, ContainerBridge, and RequestProcessor unchanged.
​3. Stage 3 — Heavy Load / Thousands of Concurrent Users: Isolate slow or blocking work (such as SMTP email delivery or heavy report generation) from the network loop, tune ConnectionPool sizing and MariaDB connection limits against max_connections, and scale vertically or horizontally as needed.
​Swapping select() for epoll Inside ServerFunctor
​select() can be replaced with epoll inside ServerFunctor.cpp on Linux without changing ServerFunctor's public interface, m_process(current) callback contract, Connection::ConnectContainer, ContainerBridge, or RequestProcessor:
​Boundary Stays at ConnectContainer&: ServerFunctor reads incoming socket data into ConnectContainer, invokes m_process(current) (ContainerBridge -> RequestProcessor), and queues the resulting response buffer back out over the socket.
​No FD_SETSIZE Ceiling: epoll_create1(EPOLL_CLOEXEC) is not restricted by FD_SETSIZE, and epoll_wait() returns only sockets with active events rather than scanning all descriptors on every iteration.
​Direct Connection Lookup: Storing the socket descriptor or connection identifier in epoll_event::data allows direct lookup when epoll_wait() reports readiness.
​Transition Steps
​1. Create and Register the epoll Instance: Open an epoll descriptor with epoll_create1(EPOLL_CLOEXEC) (and clean it up on exit), register listenFd with EPOLLIN, and register each accepted client socket with EPOLLIN (adding/modifying EPOLLOUT via epoll_ctl(..., EPOLL_CTL_MOD, ...) only when pending writes exist).
​2. Replace select() and fd_set Loops with epoll_wait(): Use epoll_wait() (preserving EINTR handling) to:
​Accept new client connections when listenFd has EPOLLIN and register them with epoll_ctl(..., EPOLL_CTL_ADD, ...).
​Read via conn->rx() on client EPOLLIN, close/remove disconnected or "exit" sockets, and push ConnectContainer items onto m_processQueue.
​Flush pending writes via conn->sx(...) on client EPOLLOUT, and use EPOLL_CTL_MOD to clear EPOLLOUT once a connection's write queue becomes empty.
​3. Preserve m_processQueue and Socket Cleanup: Keep the m_processQueue processing step (m_process(current) -> queueWrite(...)) intact, and remove descriptors with epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr) before closing sockets.
