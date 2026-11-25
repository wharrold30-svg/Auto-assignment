#ifndef AA_DATA_CONNECTION_POOL_H
#define AA_DATA_CONNECTION_POOL_H

#include <mysql++/mysql++.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Log.h"

namespace AA::Data {

class ConnectionPool final : public mysqlpp::ConnectionPool {
 public:
  struct Config {
    std::string server;
    std::string user;
    std::string password;
    std::string dbName{"Auto_Assignment"};
    unsigned int maxIdleTimeSec{60};
  };

  static Config loadConfigFromFile(
      const std::filesystem::path& path = std::filesystem::path{"creds"} /
                                          "dbcreds.txt") {
    // Safety Check: Verify that the file actually exists down the line
    if (!std::filesystem::exists(path)) {
      LoadErrorLog("Could not load database");
      throw std::runtime_error{"Could not load database"};
    }

    std::string buf;
    try {
      // Open the file for reading (ifstream)
      std::ifstream infile(path);
      if (!infile.is_open()) {
        LoadErrorLog("Could not load database");
        throw std::runtime_error{"Could not load database"};
      }

      // Dump the entire file into the string
      infile.seekg(0, std::ios::end);
      const std::streampos fileSize = infile.tellg();

      // Check if tellg() failed or returned an invalid position
      if (fileSize == -1) {
        LoadErrorLog("Could not determine file size");
        throw std::runtime_error{"Could not determine file size"};
      }

      buf.reserve(static_cast<std::size_t>(fileSize));

      // Return to the beginning and read
      infile.seekg(0, std::ios::beg);
      buf.assign((std::istreambuf_iterator<char>(infile)),
                 std::istreambuf_iterator<char>());
    } catch (const std::filesystem::filesystem_error&) {
      LoadErrorLog("Could not load database");
      throw std::runtime_error{"Could not load database"};
    }

    // Parse key=value entries anchored at start of buffer, newline, or comma
    // so prefixed keys (e.g., DB_USER=) never collide with USER=.
    auto findValue = [&buf](std::string_view key) -> std::string {
      std::size_t searchPos = 0;
      while (searchPos < buf.size()) {
        const std::size_t beg = buf.find(key, searchPos);
        if (beg == std::string::npos) {
          return {};
        }

        const bool atBoundary = (beg == 0) || (buf[beg - 1] == '\n') ||
                                (buf[beg - 1] == '\r') || (buf[beg - 1] == ',');
        if (!atBoundary) {
          searchPos = beg + key.size();
          continue;
        }

        const std::size_t valStart = beg + key.size();
        const std::size_t delimPos = buf.find_first_of(",\r\n", valStart);
        const std::size_t valEnd =
            (delimPos == std::string::npos) ? buf.size() : delimPos;
        std::string val = buf.substr(valStart, valEnd - valStart);

        // Trim any accidental spaces, tabs, or newlines from both ends
        const std::size_t first = val.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
          return {};
        }
        const std::size_t last = val.find_last_not_of(" \t\r\n");
        return val.substr(first, last - first + 1);
      }
      return {};
    };

    Config cfg;
    cfg.server = findValue("SERVER=");
    cfg.user = findValue("USER=");
    cfg.password = findValue("PASSWORD=");

    const std::string parsedDbName = findValue("DB_NAME=");
    if (!parsedDbName.empty()) {
      cfg.dbName = parsedDbName;
    }

    if (cfg.server.empty() || cfg.user.empty()) {
      LoadErrorLog("Incomplete database credentials in dbcreds.txt");
      throw std::runtime_error{"Incomplete database credentials"};
    }
    return cfg;
  }

  explicit ConnectionPool(Config config = loadConfigFromFile())
      : m_config{std::move(config)} {}

  ~ConnectionPool() override { clear(); }

  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;
  ConnectionPool(ConnectionPool&&) = delete;
  ConnectionPool& operator=(ConnectionPool&&) = delete;

 protected:
  mysqlpp::Connection* create() override {
    // Manage connection ownership with unique_ptr so exceptions never leak
    auto conn = std::make_unique<mysqlpp::Connection>(false);

    // Enforce utf8mb4 on the wire to match the database character set
    conn->set_option(new mysqlpp::SetCharsetNameOption("utf8mb4"));

    // 1. Connect to the MySQL/MariaDB server without selecting a database yet
    if (!conn->connect(nullptr, m_config.server.c_str(), m_config.user.c_str(),
                       m_config.password.c_str())) {
      AA::SaveErrorLog(std::string("MySQL connection failed: ") +
                       conn->error());
      return nullptr;
    }

    // Create schema once across all threads; if it fails, log and return
    // nullptr
    try {
      std::call_once(m_schemaOnceFlag,
                     [this, &conn]() { ensureDatabaseAndSchema(*conn); });
    } catch (const std::exception& ex) {
      AA::SaveErrorLog(std::string("Schema setup failed: ") + ex.what());
      return nullptr;
    }

    // Switch this pooled connection into the target database
    if (!conn->select_db(m_config.dbName)) {
      AA::SaveErrorLog(std::string("Failed to select database: ") +
                       conn->error());
      return nullptr;
    }

    return conn.release();
  }

  void destroy(mysqlpp::Connection* cp) override { delete cp; }

  unsigned int max_idle_time() override { return m_config.maxIdleTimeSec; }

 private:
  void ensureDatabaseAndSchema(mysqlpp::Connection& conn) {
    // 2. Skip CREATE DATABASE for pre-created test DB where user privileges
    //    are scoped to that database; otherwise create it if permitted.
    if (m_config.dbName != "auto_assignment_test") {
      mysqlpp::Query dbQuery = conn.query();
      dbQuery << "CREATE DATABASE IF NOT EXISTS `" << m_config.dbName
              << "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
      if (!dbQuery.execute()) {
        AA::SaveErrorLog(std::string("Failed to create database: ") +
                         dbQuery.error());
        throw std::runtime_error{"Failed to create database"};
      }
    }

    if (!conn.select_db(m_config.dbName)) {
      AA::SaveErrorLog(std::string("Failed to select database: ") +
                       conn.error());
      throw std::runtime_error{"Failed to select database"};
    }

    // 3. All 11 schema statements in exact foreign-key dependency order
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
            salt VARCHAR(128) NOT NULL,
            password TEXT NOT NULL,
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
      mysqlpp::Query stmt = conn.query(std::string{sql});
      if (!stmt.execute()) {
        AA::SaveErrorLog(std::string("Schema initialization failed: ") +
                         stmt.error());
        throw std::runtime_error{"Schema initialization failed"};
      }
    }
  }

  Config m_config;
  std::once_flag m_schemaOnceFlag;
};

}  // namespace AA::Data

#endif  // AA_DATA_CONNECTION_POOL_H
