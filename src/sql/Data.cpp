#include "sql/Data.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Log.h"
#include "OpenTest.h"
#include "mysql++/mysql++.h"
#include "nlohmann/json.hpp"
namespace fs = std::filesystem;
void AA::Data::Data::getDBCreds(std::string& server, std::string& user,
                                std::string& password) {
  fs::path target_dir = "creds";
  fs::path file_name = "dbcreds.txt";
  fs::path full_path = target_dir / file_name;

  // Safety Check: Verify that the file actually exists down the line
  if (!fs::exists(full_path)) {
    LoadErrorLog("Could not load database");
    throw std::runtime_error{"Could not load database"};
  }
  std::string buf;
  try {
    // Open the file for reading (ifstream)
    std::ifstream infile(full_path);

    if (infile.is_open()) {
      // dump the entire file into the string
      infile.seekg(0, std::ios::end);
      // Capture the position
      std::streampos fileSize = infile.tellg();

      // Check if tellg() failed or returned an invalid position
      if (fileSize != -1) {
        buf.reserve(static_cast<size_t>(fileSize));

        // Return to the beginning and read
        infile.seekg(0, std::ios::beg);
        buf.assign((std::istreambuf_iterator<char>(infile)),
                   std::istreambuf_iterator<char>());
        infile.close();
      } else {
        LoadErrorLog("Could not determine file size");
        throw std::runtime_error{"Could not determine file size"};
      }
    } else {
      LoadErrorLog("Could not load database");
      throw std::runtime_error{"Could not load database"};
    }
  } catch (const fs::filesystem_error& e) {
    LoadErrorLog("Could not load database");
    throw std::runtime_error{"Could not load database"};
  }

  // this function is for parsing the file and the getting the values needed
  auto findValue = [&buf](std::string_view str) -> std::string {
    std::size_t beg = buf.find(str);
    if (beg == std::string::npos) return std::string{};

    beg += str.length();  // adjust based on whether you want to skip the search
                          // term
    std::size_t end = buf.find(",", beg);

    if (end == std::string::npos) {
      end = buf.size();
    }

    // Extract the substring
    std::string val = buf.substr(beg, end - beg);

    // Trim any accidental spaces, tabs, or newlines from both ends
    val.erase(0, val.find_first_not_of(" \t\r\n"));
    val.erase(val.find_last_not_of(" \t\r\n") + 1);

    return val;
  };

  server = findValue("SERVER=");
  user = findValue("USER=");
  password = findValue("PASSWORD=");
}

bool AA::Data::Data::initializeDatabase(const std::string& server,
                                        const std::string& user,
                                        const std::string& password,
                                        const std::string& dbName) {
  // 1. m_connect to the MySQL server without selecting a database yet
  if (!m_conn.connected()) {
    if (!m_conn.connect(nullptr, server.c_str(), user.c_str(),
                        password.c_str())) {
      AA::SaveErrorLog(std::string("MySQL m_connection failed: ") +
                       m_conn.error());
      return false;
    }
  }

  // 2. Create the database if it does not exist, then switch into it
  mysqlpp::Query dbQuery = m_conn.query();
  dbQuery << "CREATE DATABASE IF NOT EXISTS `" << dbName
          << "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";

  if (!dbQuery.execute()) {
    AA::SaveErrorLog(std::string("Failed to create database: ") +
                     dbQuery.error());
    return false;
  }

  if (!m_conn.select_db(dbName)) {
    AA::SaveErrorLog(std::string("Failed to select database: ") +
                     m_conn.error());
    return false;
  }

  // 3. All 10 schema statements in exact foreign-key dependency order
  static constexpr std::array<std::string_view, 11> kSchemaStatements = {
      {// 1. Schools
       R"SQL(
        CREATE TABLE IF NOT EXISTS schools (
            name VARCHAR(150) PRIMARY KEY,
            address VARCHAR(255) NOT NULL,
            gradebook VARCHAR(100)
        )
        )SQL",

       // 2. Teachers
       R"SQL(
        CREATE TABLE IF NOT EXISTS teachers (
            teacher_id VARCHAR(64) PRIMARY KEY,
            l_name VARCHAR(100) NOT NULL,
            f_name VARCHAR(100) NOT NULL,
            email_addr VARCHAR(255) NOT NULL UNIQUE,
            school_name VARCHAR(150) NOT NULL,
            FOREIGN KEY (school_name) REFERENCES schools(name)
                ON UPDATE CASCADE
                ON DELETE RESTRICT
        )
        )SQL",

       // 3. Students
       R"SQL(
        CREATE TABLE IF NOT EXISTS students (
            student_id VARCHAR(64) PRIMARY KEY,
            email_addr VARCHAR(255) NOT NULL UNIQUE,
            l_name VARCHAR(100) NOT NULL,
            f_name VARCHAR(100) NOT NULL,
            class_period INT UNSIGNED NOT NULL DEFAULT 0,
            school_name VARCHAR(150) NOT NULL,
            tests_json TEXT,
            FOREIGN KEY (school_name) REFERENCES schools(name)
                ON UPDATE CASCADE
                ON DELETE RESTRICT
        )
        )SQL",

       // 4. Tests
       R"SQL(
        CREATE TABLE IF NOT EXISTS tests (
            test_id VARCHAR(32) PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            teacher_id VARCHAR(64) NOT NULL,
            sent_date VARCHAR(32),
            due_date VARCHAR(32),
            subject VARCHAR(100),
            description TEXT,
            level INT UNSIGNED NOT NULL DEFAULT 0,
            FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id)
                ON UPDATE CASCADE
                ON DELETE RESTRICT
        )
        )SQL",

       // 5. Test questions
       R"SQL(
        CREATE TABLE IF NOT EXISTS test_questions (
            id INT AUTO_INCREMENT PRIMARY KEY,
            test_id VARCHAR(32) NOT NULL,
            question TEXT NOT NULL,
            answer TEXT NOT NULL,
            FOREIGN KEY (test_id) REFERENCES tests(test_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE
        )
        )SQL",

       // 6. Student test results
       R"SQL(
        CREATE TABLE IF NOT EXISTS student_tests (
            id INT AUTO_INCREMENT PRIMARY KEY,
            test_id VARCHAR(32) NOT NULL,
            student_id VARCHAR(64) NOT NULL,
            return_test TEXT,
            wrong_answers TEXT,
            grade FLOAT NOT NULL DEFAULT 0.0,
            rcv_date VARCHAR(32),
            UNIQUE KEY unique_student_test (test_id, student_id),
            FOREIGN KEY (test_id) REFERENCES tests(test_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE,
            FOREIGN KEY (student_id) REFERENCES students(student_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE
        )
        )SQL",

       // 7. Teacher roster (Teacher -> Students + Period)
       R"SQL(
        CREATE TABLE IF NOT EXISTS teacher_roster (
            id INT AUTO_INCREMENT PRIMARY KEY,
            teacher_id VARCHAR(64) NOT NULL,
            student_id VARCHAR(64) NOT NULL,
            class_period INT UNSIGNED NOT NULL,
            UNIQUE KEY unique_enrollment (teacher_id, student_id, class_period),
            FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE,
            FOREIGN KEY (student_id) REFERENCES students(student_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE
        )
        )SQL",

       // 8. Student schedule view (Student -> Teachers + Period)
       R"SQL(
        CREATE OR REPLACE VIEW student_schedule AS
        SELECT id, student_id, teacher_id, class_period
        FROM teacher_roster
        )SQL",

       // 9. Open tests
       R"SQL(
        CREATE TABLE IF NOT EXISTS open_tests (
            id INT AUTO_INCREMENT PRIMARY KEY,
            test_id VARCHAR(32) NOT NULL,
            teacher_id VARCHAR(64) NOT NULL,
            creation_date VARCHAR(32) NOT NULL,
            due_date VARCHAR(32) NOT NULL,
            duration INT UNSIGNED NOT NULL DEFAULT 0,
            FOREIGN KEY (test_id) REFERENCES tests(test_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE,
            FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id)
                ON UPDATE CASCADE
                ON DELETE CASCADE
        )
        )SQL",

       // 10. Student open test log
       R"SQL(
CREATE TABLE IF NOT EXISTS student_open_test_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    student_id VARCHAR(64) NOT NULL,
    open_test_id INT NOT NULL,
    status VARCHAR(32) NOT NULL DEFAULT 'Pending',
    submission_date VARCHAR(32) NULL,
    score DOUBLE DEFAULT 0.0,
    class_period INT UNSIGNED NOT NULL,
    UNIQUE KEY unique_student_open_test (student_id, open_test_id),
    FOREIGN KEY (student_id) REFERENCES students(student_id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,
    FOREIGN KEY (open_test_id) REFERENCES open_tests(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
      )
      )SQL",
       // 11. Gradebooks
       R"SQL(
        CREATE TABLE IF NOT EXISTS gradebooks (
            name VARCHAR(100) PRIMARY KEY,
            url VARCHAR(512) NOT NULL,
            post_temp_script TEXT NOT NULL
        )
        )SQL"}};

  // 4. Execute each statement sequentially
  for (const auto& sql : kSchemaStatements) {
    mysqlpp::Query stmt = m_conn.query(std::string{sql});
    if (!stmt.execute()) {
      AA::SaveErrorLog(std::string("Schema initialization failed: ") +
                       stmt.error());
      return false;
    }
  }

  return true;
}

void AA::Data::Data::Init() {
  std::string server, user, password;
  getDBCreds(server, user, password);
  initializeDatabase(server, user, password);
}

AA::Data::Data::Data() { Init(); }
AA::Data::Data::~Data() {
  if (m_conn.connected()) {
    m_conn.disconnect();
  }
}

////////////////////////////////////load////////////////////////

//////Student
bool AA::Data::Data::loadStudent(const std::string& student_id,
                                 AA::Student& out_student) {
  if (!m_conn.connected()) return false;

  // 1. Query the raw student SSQLS row
  mysqlpp::Query query = m_conn.query();
  query << "SELECT * FROM students WHERE student_id = " << mysqlpp::quote
        << student_id;

  std::vector<SSQLS::students> rows;
  query.storein(rows);
  if (rows.empty()) {
    return false;  // Student not found
  }

  // 2. Fetch the related School object using its foreign key
  // (rows[0].school_name)
  AA::School school;
  if (!loadSchool(rows[0].school_name, school)) {
    // Handle missing school fallback if necessary
    school = AA::School(rows[0].school_name, "", "");
  }

  // 3. Convert SSQLS row -> AA::Student domain object using fromSSQLS()
  // (This automatically handles parsing the tests_json string into
  // std::vector<Test>)
  out_student = AA::Student::fromSSQLS(rows[0], school);

  return true;
}

/////Test
bool AA::Data::Data::loadTest(const std::string& test_id, AA::Test& out_test) {
  if (!m_conn.connected()) return false;

  // 1. Load Test Metadata
  mysqlpp::Query test_query = m_conn.query();
  test_query << "SELECT * FROM tests WHERE test_id = " << mysqlpp::quote
             << test_id;
  std::vector<SSQLS::tests> test_rows;
  test_query.storein(test_rows);
  if (test_rows.empty()) return false;

  // Fetch the associated Teacher
  AA::Teacher teacher;
  if (!loadTeacher(test_rows[0].teacher_id, teacher)) {
    return false;
  }

  // Build base test object using fromSSQLS
  out_test = AA::Test::fromSSQLS(test_rows[0], teacher);
  out_test.setID(test_rows[0].test_id);

  // 2. Load Test Questions (TestVec / QandA)
  mysqlpp::Query q_query = m_conn.query();
  q_query << "SELECT * FROM test_questions WHERE test_id = " << mysqlpp::quote
          << test_id;
  std::vector<SSQLS::test_questions> q_rows;
  q_query.storein(q_rows);

  AA::TestVec questions;
  questions.reserve(q_rows.size());
  for (const auto& q_row : q_rows) {
    questions.push_back(AA::QandA::fromSSQLS(q_row));
  }
  out_test.setTest(questions);

  // 3. Load Student Submissions (studentExt / student_tests)
  mysqlpp::Query sub_query = m_conn.query();
  sub_query << "SELECT * FROM student_tests WHERE test_id = " << mysqlpp::quote
            << test_id;
  std::vector<SSQLS::student_tests> sub_rows;
  sub_query.storein(sub_rows);

  for (const auto& sub_row : sub_rows) {
    // Load the base student profile
    auto student_ptr = std::make_shared<AA::Student>();
    if (!loadStudent(sub_row.student_id, *student_ptr)) {
      continue;
    }

    // Register student with the test using your public addStudent method
    out_test.addStudent(student_ptr, sub_row.id);

    // Populate submission details (return test, wrong answers, grade, rcv date)
    nlohmann::json j_ret =
        nlohmann::json::parse(sub_row.return_test, nullptr, false);
    std::vector<std::string> returnTestVec;
    if (!j_ret.is_discarded() && j_ret.is_array()) {
      returnTestVec = j_ret.get<std::vector<std::string>>();
    }

    DT::Date rcvDate(sub_row.rcv_date);
    out_test.setStudentTest(student_ptr, returnTestVec, rcvDate, sub_row.id);
  }

  return true;
}

//////Teacher
bool AA::Data::Data::loadTeacher(const std::string& teacher_id,
                                 AA::Teacher& out_teacher) {
  if (!m_conn.connected()) return false;

  // 1. Query the raw teacher SSQLS row
  mysqlpp::Query query = m_conn.query();
  query << "SELECT * FROM teachers WHERE teacher_id = " << mysqlpp::quote
        << teacher_id;

  std::vector<SSQLS::teachers> rows;
  query.storein(rows);
  if (rows.empty()) {
    return false;  // Teacher not found
  }

  // 2. Fetch the related School object using its foreign key
  // (rows[0].school_name)
  AA::School school;
  if (!loadSchool(rows[0].school_name, school)) {
    school = AA::School(rows[0].school_name, "",
                        "");  // Fallback if school metadata is missing
  }

  // 3. Rebuild the Teacher domain object with the nested School composition
  out_teacher = AA::Teacher::fromSSQLS(rows[0], school);

  return true;
}

/////School
bool AA::Data::Data::loadSchool(const std::string& school_name,
                                AA::School& out_school) {
  if (!m_conn.connected()) return false;

  // 1. Query the raw schools table using safe quoting
  mysqlpp::Query query = m_conn.query();
  query << "SELECT * FROM schools WHERE name = " << mysqlpp::quote
        << school_name;

  std::vector<SSQLS::schools> rows;
  query.storein(rows);
  if (rows.empty()) {
    return false;  // School not found
  }

  // 2. Convert the SSQLS row back to your School domain object via fromSSQLS()
  out_school = AA::School::fromSSQLS(rows[0]);

  return true;
}

//////////////////////////////////saves/////////////////////////

/////School
bool AA::Data::Data::saveSchool(const AA::School& school) {
  if (!m_conn.connected()) return false;

  // 1. Convert domain object to flat SSQLS row representation
  SSQLS::schools row = school.toSSQLS();

  // 2. Perform UPSERT (Insert or update if the school name already exists)
  mysqlpp::Query query = m_conn.query();
  query << "INSERT INTO schools (name, address, gradebook) VALUES ("
        << mysqlpp::quote << row.name << ", " << mysqlpp::quote << row.address
        << ", " << mysqlpp::quote << row.gradebook << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "address = VALUES(address), "
        << "gradebook = VALUES(gradebook)";

  return query.execute();
}

////Test///
bool AA::Data::Data::saveTest(const AA::Test& test) {
  if (!m_conn.connected()) return false;

  // Use a transaction so if any part fails, changes are rolled back
  mysqlpp::Transaction trans(m_conn);

  try {
    // 1. Save Test Metadata
    SSQLS::tests test_row = test.toSSQLS();
    mysqlpp::Query meta_query = m_conn.query();
    meta_query << "INSERT INTO tests (test_id, name, teacher_id, sent_date, "
                  "due_date, subject, description, level) "
               << "VALUES (" << mysqlpp::quote << test_row.test_id << ", "
               << mysqlpp::quote << test_row.name << ", " << mysqlpp::quote
               << test_row.teacher_id << ", " << mysqlpp::quote
               << test_row.sent_date << ", " << mysqlpp::quote
               << test_row.due_date << ", " << mysqlpp::quote
               << test_row.subject << ", " << mysqlpp::quote
               << test_row.description << ", " << test_row.level
               << ") ON DUPLICATE KEY UPDATE "
               << "name = VALUES(name), "
               << "teacher_id = VALUES(teacher_id), "
               << "sent_date = VALUES(sent_date), "
               << "due_date = VALUES(due_date), "
               << "subject = VALUES(subject), "
               << "description = VALUES(description), "
               << "level = VALUES(level)";
    meta_query.execute();

    // 2. Sync Test Questions (Clear old questions for this test and re-insert
    // current ones)
    mysqlpp::Query clear_q = m_conn.query();
    clear_q << "DELETE FROM test_questions WHERE test_id = " << mysqlpp::quote
            << test_row.test_id;
    clear_q.execute();

    for (const auto& q : test.getTest()) {
      SSQLS::test_questions q_row = q.toSSQLS();
      // Ensure the question row links back to this test's ID
      q_row.test_id = test_row.test_id;

      mysqlpp::Query q_query = m_conn.query();
      q_query
          << "INSERT INTO test_questions (test_id, question, answer) VALUES ("
          << mysqlpp::quote << q_row.test_id << ", " << mysqlpp::quote
          << q_row.question << ", " << mysqlpp::quote << q_row.answer << ")";
      q_query.execute();
    }

    // Commit transaction successfully
    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    std::cerr << "Database error saving test: " << e.what() << std::endl;
    // Transaction automatically rolls back if not committed before going out of
    // scope
    return false;
  }
}

//////Student
bool AA::Data::Data::saveStudent(const AA::Student& student) {
  if (!m_conn.connected()) return false;

  // 1. Convert domain object to its SSQLS database row representation
  SSQLS::students row = student.toSSQLS();

  // 2. Construct an UPSERT query using MySQL++ quoting to prevent SQL injection
  mysqlpp::Query query = m_conn.query();
  query << "INSERT INTO students (student_id, email_addr, l_name, f_name, "
           "class_period, school_name, tests_json) "
        << "VALUES (" << mysqlpp::quote << row.student_id << ", "
        << mysqlpp::quote << row.email_addr << ", " << mysqlpp::quote
        << row.l_name << ", " << mysqlpp::quote << row.f_name << ", "
        << row.class_period << ", " << mysqlpp::quote << row.school_name << ", "
        << mysqlpp::quote << row.tests_json << ") ON DUPLICATE KEY UPDATE "
        << "email_addr = VALUES(email_addr), "
        << "l_name = VALUES(l_name), "
        << "f_name = VALUES(f_name), "
        << "class_period = VALUES(class_period), "
        << "school_name = VALUES(school_name), "
        << "tests_json = VALUES(tests_json)";

  // 3. Execute the query
  return query.execute();
}

//////Teacher
bool AA::Data::Data::saveTeacher(const AA::Teacher& teacher) {
  if (!m_conn.connected()) return false;

  // Use a transaction so both the school and teacher persist safely together
  mysqlpp::Transaction trans(m_conn);

  try {
    // 1. Ensure the nested School exists first (satisfies foreign key
    // constraint) (Assuming your Teacher class has a getter like getSchool())
    AA::School school = teacher.getSchool();
    SSQLS::schools school_row = school.toSSQLS();

    mysqlpp::Query school_query = m_conn.query();
    school_query << "INSERT INTO schools (name, address, gradebook) VALUES ("
                 << mysqlpp::quote << school_row.name << ", " << mysqlpp::quote
                 << school_row.address << ", " << mysqlpp::quote
                 << school_row.gradebook << ") "
                 << "ON DUPLICATE KEY UPDATE "
                 << "address = VALUES(address), "
                 << "gradebook = VALUES(gradebook)";
    school_query.execute();

    // 2. Convert the Teacher domain object to its flat SSQLS row representation
    SSQLS::teachers teacher_row = teacher.toSSQLS();

    // 3. Perform UPSERT for the Teacher record using safe quoting
    mysqlpp::Query teacher_query = m_conn.query();
    teacher_query << "INSERT INTO teachers (teacher_id, l_name, f_name, "
                     "email_addr, school_name) "
                  << "VALUES (" << mysqlpp::quote << teacher_row.teacher_id
                  << ", " << mysqlpp::quote << teacher_row.l_name << ", "
                  << mysqlpp::quote << teacher_row.f_name << ", "
                  << mysqlpp::quote << teacher_row.email_addr << ", "
                  << mysqlpp::quote << teacher_row.school_name << ") "
                  << "ON DUPLICATE KEY UPDATE "
                  << "l_name = VALUES(l_name), "
                  << "f_name = VALUES(f_name), "
                  << "email_addr = VALUES(email_addr), "
                  << "school_name = VALUES(school_name)";
    teacher_query.execute();

    // Commit both operations successfully
    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    std::cerr << "Database error saving teacher: " << e.what() << std::endl;
    // Transaction automatically rolls back if an exception occurs
    return false;
  }
}

bool AA::Data::Data::enrollStudentInClass(const std::string& teacherId,
                                          const std::string& studentId,
                                          unsigned int classPeriod) {
  if (!m_conn.connected()) return false;

  // Use a transaction to ensure foreign key lookups and inserts are safe
  mysqlpp::Transaction trans(m_conn);

  try {
    mysqlpp::Query query = m_conn.query();

    // Insert or update the enrollment mapping
    query
        << "INSERT INTO teacher_roster (teacher_id, student_id, class_period) "
        << "VALUES (" << mysqlpp::quote << teacherId << ", " << mysqlpp::quote
        << studentId << ", " << classPeriod
        << ") "  // Numbers don't require mysqlpp::quote
        << "ON DUPLICATE KEY UPDATE "
        << "class_period = VALUES(class_period)";

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to save student enrollment: ") +
                       query.error());
      return false;
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception saving student enrollment: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::saveTeacherRoster(
    const std::string& teacherId,
    const std::vector<std::pair<std::string, unsigned int>>&
        studentPeriodPairs) {
  if (!m_conn.connected()) return false;

  mysqlpp::Transaction trans(m_conn);

  try {
    for (const auto& [studentId, period] : studentPeriodPairs) {
      mysqlpp::Query query = m_conn.query();
      query << "INSERT INTO teacher_roster (teacher_id, student_id, "
               "class_period) "
            << "VALUES (" << mysqlpp::quote << teacherId << ", "
            << mysqlpp::quote << studentId << ", " << period << ") "
            << "ON DUPLICATE KEY UPDATE "
            << "class_period = VALUES(class_period)";

      if (!query.execute()) {
        // Rollback occurs automatically if transaction is not committed
        return false;
      }
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Batch roster save failed: ") + e.what());
    return false;
  }
}

bool AA::Data::Data::loadTeacherRoster(
    const std::string& teacherId,
    std::vector<std::pair<std::string, unsigned int>>& out_roster) {
  if (!m_conn.connected()) return false;

  out_roster.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    query << "SELECT student_id, class_period FROM teacher_roster WHERE "
             "teacher_id = "
          << mysqlpp::quote << teacherId << " ORDER BY class_period ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      std::string studentId = res[i]["student_id"].c_str();
      unsigned int period = res[i]["class_period"];
      out_roster.emplace_back(studentId, period);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading teacher roster: ") + e.what());
    return false;
  }
}

bool AA::Data::Data::loadStudentSchedule(
    const std::string& studentId,
    std::vector<std::tuple<std::string, unsigned int>>& out_schedule) {
  if (!m_conn.connected()) return false;

  out_schedule.clear();
  try {
    // student_schedule view contains: id, student_id, teacher_id, class_period
    mysqlpp::Query query = m_conn.query();
    query << "SELECT teacher_id, class_period FROM student_schedule WHERE "
             "student_id = "
          << mysqlpp::quote << studentId << " ORDER BY class_period ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      std::string teacherId = res[i]["teacher_id"].c_str();
      unsigned int period = res[i]["class_period"];
      out_schedule.emplace_back(teacherId, period);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading student schedule: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::saveOpenTest(const AA::OpenTest& openTest) {
  if (!m_conn.connected()) return false;

  try {
    mysqlpp::Query query = m_conn.query();
    query << "INSERT INTO open_tests (test_id, teacher_id, creation_date, "
             "due_date, duration) "
          << "VALUES (" << mysqlpp::quote << openTest.getTestID() << ", "
          << mysqlpp::quote << openTest.getTeacherID() << ", " << mysqlpp::quote
          << openTest.getCreationDate().toStringSql() << ", " << mysqlpp::quote
          << openTest.getDueDate().toStringSql() << ", "
          << openTest.getDuration() << ")";

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to save open test: ") +
                       query.error());
      return false;
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception saving open test: ") + e.what());
    return false;
  }
}

bool AA::Data::Data::loadOpenTestsByDateRange(
    const DT::Date& startDate, const DT::Date& endDate,
    std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();

    // Construct the query using BETWEEN with safely quoted DT::Date strings
    query
        << "SELECT id, test_id, teacher_id, creation_date, due_date, duration "
        << "FROM open_tests "
        << "WHERE due_date BETWEEN " << mysqlpp::quote
        << startDate.toStringSql() << " AND " << mysqlpp::quote
        << endDate.toStringSql() << " ORDER BY due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    // Map the result rows back into your OpenTestRecord structs
    for (size_t i = 0; i < res.num_rows(); ++i) {
      AA::OpenTest record;
      record.setID(res[i]["id"]);
      record.setTestID(res[i]["test_id"].c_str());
      record.setTeacherID(res[i]["teacher_id"].c_str());
      record.setCreationDate(DT::Date(res[i]["creation_date"].c_str()));
      record.setDueDate(DT::Date(res[i]["due_date"].c_str()));
      record.setDuration(res[i]["duration"]);

      out_tests.push_back(record);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests by date range: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::loadOpenTestsForTeacher(
    const std::string& teacherId, std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    query
        << "SELECT id, test_id, teacher_id, creation_date, due_date, duration "
        << "FROM open_tests WHERE teacher_id = " << mysqlpp::quote << teacherId
        << " ORDER BY due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      int id = res[i]["id"];
      std::string testID = res[i]["test_id"].c_str();
      std::string tID = res[i]["teacher_id"].c_str();

      // Parse database strings into DT::Date
      DT::Date creationDate(res[i]["creation_date"].c_str());
      DT::Date dueDate(res[i]["due_date"].c_str());

      unsigned int duration = res[i]["duration"];

      // Construct using your OpenTest constructor
      out_tests.emplace_back(id, testID, tID, creationDate, dueDate, duration);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests for teacher: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::loadOpenTestsForStudent(
    const std::string& studentId, std::vector<AA::OpenTest>& out_tests) {
  if (!m_conn.connected()) return false;

  out_tests.clear();
  try {
    mysqlpp::Query query = m_conn.query();
    // Join the student's log table with the open_tests table
    query << "SELECT ot.id, ot.test_id, ot.teacher_id, ot.creation_date, "
             "ot.due_date, ot.duration "
          << "FROM student_open_test_log sotl "
          << "JOIN open_tests ot ON sotl.open_test_id = ot.id "
          << "WHERE sotl.student_id = " << mysqlpp::quote << studentId
          << " ORDER BY ot.due_date ASC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      int id = res[i]["id"];
      std::string testID = res[i]["test_id"].c_str();
      std::string tID = res[i]["teacher_id"].c_str();

      DT::Date creationDate(res[i]["creation_date"].c_str());
      DT::Date dueDate(res[i]["due_date"].c_str());

      unsigned int duration = res[i]["duration"];

      out_tests.emplace_back(id, testID, tID, creationDate, dueDate, duration);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading open tests for student: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::assignOpenTestToStudent(const std::string& studentId,
                                             int openTestId) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to assign open test.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();

    // Insert the relationship row.
    // student_id is a string (requires mysqlpp::quote), openTestId is an int
    // (does not).
    query
        << "INSERT INTO student_open_test_log (student_id, open_test_id) "
        << "VALUES (" << mysqlpp::quote << studentId << ", " << openTestId
        << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "open_test_id = VALUES(open_test_id)";  // No-op update to prevent
                                                   // errors if already assigned

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to assign open test to student: ") +
                       query.error());
      return false;
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(
        std::string("MySQL exception while assigning open test: ") + e.what());
    return false;
  }
}

bool AA::Data::Data::assignOpenTestToMultipleStudents(
    const std::vector<std::string>& studentIds, int openTestId) {
  if (!m_conn.connected()) return false;

  mysqlpp::Transaction trans(m_conn);

  try {
    for (const auto& studentId : studentIds) {
      mysqlpp::Query query = m_conn.query();
      query << "INSERT INTO student_open_test_log (student_id, open_test_id) "
            << "VALUES (" << mysqlpp::quote << studentId << ", " << openTestId
            << ") "
            << "ON DUPLICATE KEY UPDATE "
            << "open_test_id = VALUES(open_test_id)";

      if (!query.execute()) {
        return false;  // Transaction automatically rolls back on scope exit if
                       // not committed
      }
    }

    trans.commit();
    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Batch open test assignment failed: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::returnStudentTest(const std::string& studentId,
                                       int openTestId,
                                       const DT::Date& submissionDate,
                                       double score,
                                       const std::string& status) {
  if (!m_conn.connected()) return false;

  try {
    mysqlpp::Query query = m_conn.query();

    query << "UPDATE student_open_test_log "
          << "SET submission_date = " << mysqlpp::quote
          << submissionDate.toStringSql() << ", "
          << "score = " << score << ", "
          << "status = " << mysqlpp::quote << status << " "
          << "WHERE student_id = " << mysqlpp::quote << studentId
          << " AND open_test_id = " << openTestId;

    if (!query.execute()) {
      AA::SaveErrorLog(std::string("Failed to return student test: ") +
                       query.error());
      return false;
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(
        std::string("MySQL exception while returning student test: ") +
        e.what());
    return false;
  }
}

bool AA::Data::Data::loadTestsForTeacher(const std::string& teacherId,
                                         std::vector<AA::Test>& out_tests) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load teacher tests.");
    return false;
  }

  out_tests.clear();
  try {
    // 1. Join tests, teachers, and schools tables to fetch all fields for the
    // entire collection
    mysqlpp::Query query = m_conn.query();
    query << "SELECT t.*, "
          << "th.l_name, th.f_name, th.email_addr, "
          << "s.name AS school_name, s.address AS school_address, s.gradebook "
             "AS school_gradebook "
          << "FROM tests t "
          << "JOIN teachers th ON t.teacher_id = th.teacher_id "
          << "JOIN schools s ON th.school_name = s.name "
          << "WHERE t.teacher_id = " << mysqlpp::quote << teacherId
          << " ORDER BY t.due_date DESC";

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) return false;

    for (size_t i = 0; i < res.num_rows(); ++i) {
      // 2. Reconstruct School object
      AA::School school(res[i]["school_name"].c_str(),
                        res[i]["school_address"].c_str(),
                        res[i]["school_gradebook"].c_str());

      // 3. Reconstruct Teacher object
      AA::Teacher teacher(res[i]["teacher_id"].c_str(),
                          res[i]["l_name"].c_str(), res[i]["f_name"].c_str(),
                          res[i]["email_addr"].c_str(), school);

      // 4. Map query row to SSQLS::tests
      SSQLS::tests test_row;
      test_row.test_id = res[i]["test_id"].c_str();
      test_row.name = res[i]["name"].c_str();
      test_row.teacher_id = res[i]["teacher_id"].c_str();
      test_row.sent_date = res[i]["sent_date"].c_str();
      test_row.due_date = res[i]["due_date"].c_str();
      test_row.subject = res[i]["subject"].c_str();
      test_row.description = res[i]["description"].c_str();
      test_row.level = res[i]["level"];

      // 5. Use your static fromSSQLS factory method to instantiate Test
      AA::Test test = AA::Test::fromSSQLS(test_row, teacher);
      test.setID(test_row.test_id);

      // 6. Load and attach all associated questions/answers from test_questions
      // table
      mysqlpp::Query q_query = m_conn.query();
      q_query << "SELECT id, test_id, question, answer FROM test_questions "
              << "WHERE test_id = " << mysqlpp::quote << test_row.test_id
              << " ORDER BY id ASC";

      std::vector<SSQLS::test_questions> q_rows;
      q_query.storein(q_rows);

      TestVec questions;
      for (const auto& q_row : q_rows) {
        questions.push_back(AA::QandA::fromSSQLS(q_row));
      }
      test.setTest(questions);

      // Add fully populated test to the output vector
      out_tests.push_back(test);
    }

    return true;

  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Error loading tests for teacher: ") +
                     e.what());
    return false;
  }
}

// this allow gettingknly so many test at a time
bool AA::Data::Data::loadTestsPaginated(int limit, int offset,
                                        std::vector<AA::Test>& out_tests) {
  out_tests.clear();

  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load paginated "
        "tests.");
    return false;
  }

  try {
    // Construct the query with LIMIT and OFFSET for pagination
    mysqlpp::Query query = m_conn.query();
    query << "SELECT t.test_id, t.name, t.subject, t.description, "
             "t.creation_date "
          << "FROM tests t "
          << "ORDER BY t.creation_date DESC "
          << "LIMIT " << limit << " OFFSET " << offset;

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    // Loop through the results and build your Test objects
    for (size_t i = 0; i < res.num_rows(); ++i) {
      AA::Test record;
      record.setTestID(res[i]["test_id"].c_str());
      // Set other fields here (name, subject, description, creation_date, etc.)
      record.setTestID(res[i]["test_id"].c_str());

      record.setTestName(res[i]["name"].c_str());

      record.setSubject(res[i]["subject"].c_str());

      record.setDescription(res[i]["description"].c_str());
      record.setSentDate(DT::Date{res[i]["sent_date"].c_str()});
      out_tests.push_back(record);
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading paginated tests: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::getStudentAssignments(
    const std::string& studentId, std::vector<GradeRecord>& out_assignments) {
  out_assignments.clear();

  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to load student "
        "assignments.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query
        << "SELECT ot.test_id, ot.name AS test_name, ot.subject, ot.due_date, "
        << "       sot.submission_date, sot.score, sot.status "
        << "FROM student_open_test_log sot "
        << "JOIN open_tests ot ON sot.open_test_id = ot.test_id "
        << "WHERE sot.student_id = " << mysqlpp::quote << studentId
        << " ORDER BY ot.due_date ASC";  // Order by due date so
                                         // pending/upcoming tests are front and
                                         // center

    mysqlpp::StoreQueryResult res = query.store();
    if (!res) {
      return false;
    }

    for (size_t i = 0; i < res.num_rows(); ++i) {
      GradeRecord record;
      record.setTestID(res[i]["test_id"].c_str());
      record.setTestName(res[i]["test_name"].c_str());
      record.setSubject(res[i]["subject"].c_str());

      // Due date is always present for open tests
      if (!res[i]["due_date"].is_null()) {
        record.setDueDate(DT::Date{res[i]["due_date"].c_str()});
      }

      // Submission date and score might be null if pending/unsubmitted
      if (!res[i]["submission_date"].is_null()) {
        record.setSubmissionDate(DT::Date{res[i]["submission_date"].c_str()});
      }
      if (!res[i]["score"].is_null()) {
        record.setGrade(res[i]["grade"]);
      }

      // e.g., "pending", "completed", "not turned in"
      record.setStatus(res[i]["status"].c_str());

      out_assignments.push_back(record);
    }

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception loading student assignments: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::removeStudentFromClass(const std::string& studentId,
                                            const std::string& teacherId,
                                            int classPeriod) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to remove student from "
        "class.");
    return false;
  }

  try {
    mysqlpp::Query query = m_conn.query();
    query << "DELETE FROM teacher_roster "
          << "WHERE student_id = " << mysqlpp::quote << studentId
          << " AND teacher_id = " << mysqlpp::quote << teacherId
          << " AND class_period = " << classPeriod;

    query.execute();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(
        std::string("Exception removing student from class roster: ") +
        e.what());
    return false;
  }
}

bool AA::Data::Data::removeStudentFromSchoolCompletely(
    const std::string& studentId, const std::string& schoolName) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to remove student from "
        "school.");
    return false;
  }

  try {
    // Deleting the student from the students table where they belong to this
    // school. Thanks to ON DELETE CASCADE in your schema, this automatically
    // deletes all matching rows in teacher_roster, student_open_test_log, and
    // student_tests!
    mysqlpp::Query query = m_conn.query();
    query << "DELETE FROM students "
          << "WHERE student_id = " << mysqlpp::quote << studentId
          << " AND school_name = " << mysqlpp::quote << schoolName;

    query.execute();
    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception removing student from school: ") +
                     e.what());
    return false;
  }
}

bool AA::Data::Data::transferStudentSchool(const std::string& studentId,
                                           const std::string& newSchoolName) {
  if (!m_conn.connected()) {
    AA::SaveErrorLog(
        "Database connection is inactive while trying to transfer student.");
    return false;
  }

  try {
    // Use a transaction so all steps succeed or roll back together
    // Note: MySQL++ transaction handling can vary, but standard query execution
    // works well.

    // Step 1: Remove the student from all previous teacher rosters
    // (since classes are tied to the old school's teachers)
    mysqlpp::Query rosterQuery = m_conn.query();
    rosterQuery << "DELETE FROM teacher_roster WHERE student_id = "
                << mysqlpp::quote << studentId;
    rosterQuery.execute();

    // Step 2: Clear old open test logs tied to the old school/teachers
    mysqlpp::Query testLogQuery = m_conn.query();
    testLogQuery << "DELETE FROM student_open_test_log WHERE student_id = "
                 << mysqlpp::quote << studentId;
    testLogQuery.execute();

    // Step 3: Update the student's school_name to the new school
    mysqlpp::Query updateQuery = m_conn.query();
    updateQuery << "UPDATE students SET school_name = " << mysqlpp::quote
                << newSchoolName << " WHERE student_id = " << mysqlpp::quote
                << studentId;
    updateQuery.execute();

    return true;
  } catch (const mysqlpp::Exception& e) {
    AA::SaveErrorLog(std::string("Exception during student school transfer: ") +
                     e.what());
    return false;
  }
}
