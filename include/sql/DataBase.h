#ifndef SQL_DATABASE_H
#define SQL_DATABASE_H
#include <mysql++/mysql++.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "Log.h"
namespace fs = std::filesystem;
namespace AA {
namespace Data {
template <typename Bridge, typename Derived, typename IdType = std::string>
class DataBase {
 private:
  void getDBCreds(std::string& server, std::string& user,
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
      if (beg == std::string::npos) {
        return std::string{};
      }

      beg += str.length();  // adjust based on whether you want to skip the
                            // search term
      const std::size_t end = buf.find(',', beg);
      const std::size_t valEnd = (end == std::string::npos) ? buf.size() : end;
      std::string val = buf.substr(beg, valEnd - beg);
      // Trim any accidental spaces, tabs, or newlines from both ends
      const std::size_t first = val.find_first_not_of(" \t\r\n");
      if (first == std::string::npos) {
        return {};
      }
      const std::size_t last = val.find_last_not_of(" \t\r\n");
      return val.substr(first, last - first + 1);
    };

    server = findValue("SERVER=");
    user = findValue("USER=");
    password = findValue("PASSWORD=");
  }
  bool initializeDatabase(const std::string& server, const std::string& user,
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
      mysqlpp::Query stmt = m_conn.query(std::string{sql});
      if (!stmt.execute()) {
        AA::SaveErrorLog(std::string("Schema initialization failed: ") +
                         stmt.error());
        return false;
      }
    }

    return true;
  }
  void Init() {
    std::string server, user, password;
    getDBCreds(server, user, password);
    initializeDatabase(server, user, password, "Auto_Assignment");
  }

 protected:
  mysqlpp::Connection& m_conn;  // Connection reference stored in the base class
  Bridge& bridge() { return *static_cast<Bridge*>(this); }
  const Bridge& bridge() const { return *static_cast<const Bridge*>(this); }

 public:
  // Constructor initializes the connection reference
  explicit DataBase(mysqlpp::Connection& conn) : m_conn{conn} {
    static bool initialized = false;
    if (!initialized && !m_conn.connected()) {
      Init();
      initialized = true;
    }
  }

  virtual ~DataBase() = default;

  // Pure virtual method for table naming
  virtual std::string getTableName() const = 0;

  // 1. SAVE / INSERT using the AA:: domain object's toSSQLS()
  bool save(const Derived& model) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query.insert(model.toSSQLS());
      query.execute();
      return true;
    } catch (const mysqlpp::Exception& e) {
      return false;
    }
  }

  // 2. UPDATE using the AA:: domain object's toSSQLS()
  bool update(const Derived& model, const std::string& idColumn,
              IdType idValue) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query.update(model.toSSQLS());
      query << " WHERE " << idColumn << " = ";

      if constexpr (std::is_same_v<std::decay_t<IdType>, std::string> ||
                    std::is_convertible_v<IdType, const char*>) {
        query << mysqlpp::quote << idValue;
      } else {
        query << idValue;
      }

      query.execute();
      return true;
    } catch (const mysqlpp::Exception& e) {
      return false;
    }
  }

  // 3. READ (Instance method: uses m_conn and the bridge's getTableName())
  bool read(const std::string& idColumn, IdType idValue,
            Derived& out_model) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query << "SELECT * FROM " << bridge().getTableName() << " WHERE "
            << idColumn << " = ";

      if constexpr (std::is_same_v<std::decay_t<IdType>, std::string> ||
                    std::is_convertible_v<IdType, const char*>) {
        query << mysqlpp::quote << idValue;
      } else {
        query << idValue;
      }

      mysqlpp::StoreQueryResult res = query.store();
      if (!res || res.num_rows() == 0) return false;

      out_model.fromSSQLS(res[0]);
      return true;
    } catch (const mysqlpp::Exception& e) {
      return false;
    }
  }

  // 4. REMOVE (Instance method: uses m_conn and the bridge's getTableName())
  bool remove(const std::string& idColumn, IdType idValue) const {
    if (!m_conn.connected()) return false;
    try {
      mysqlpp::Query query = m_conn.query();
      query << "DELETE FROM " << bridge().getTableName() << " WHERE "
            << idColumn << " = ";

      if constexpr (std::is_same_v<std::decay_t<IdType>, std::string> ||
                    std::is_convertible_v<IdType, const char*>) {
        query << mysqlpp::quote << idValue;
      } else {
        query << idValue;
      }

      query.execute();
      return true;
    } catch (const mysqlpp::Exception& e) {
      return false;
    }
  }

};  // DataBase
}  // namespace Data
}  // namespace AA
#endif
