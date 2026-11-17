#ifndef SSQLS_H
#define SSQLS_H

// Suppress static table metadata in every translation unit by default
#ifndef SSQLS_DEFINE_STATICS
#define MYSQLPP_SSQLS_NO_STATICS
#endif

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace SSQLS {

// 1. Schools table (maps to AA::School)
sql_create_3(schools, 1, 3, mysqlpp::sql_varchar,
             name,                           // AA::School::m_name (Primary Key)
             mysqlpp::sql_varchar, address,  // AA::School::m_address
             mysqlpp::sql_varchar, gradebook  // AA::School::m_gradebook
             )

    // 2. Teachers table (maps to AA::Teacher)
    sql_create_5(
        teachers, 1, 5, mysqlpp::sql_varchar,
        teacher_id,                    // AA::Teacher::m_teacherID (Primary Key)
        mysqlpp::sql_varchar, l_name,  // AA::Teacher::m_lName
        mysqlpp::sql_varchar, f_name,  // AA::Teacher::m_fName
        mysqlpp::sql_varchar, email_addr,  // AA::Teacher::m_emailAddr (Unique)
        mysqlpp::sql_varchar,
        school_name  // Foreign key -> schools.name (AA::Teacher::m_school)
        )

    // 3. Students table (maps to AA::Student)
    sql_create_7(
        students, 1, 7, mysqlpp::sql_varchar,
        student_id,  // AA::Student::m_studentID (Primary Key)
        mysqlpp::sql_varchar, email_addr,  // AA::Student::m_emailAddr (Unique)
        mysqlpp::sql_varchar, l_name,      // AA::Student::m_lName
        mysqlpp::sql_varchar, f_name,      // AA::Student::m_fName
        mysqlpp::sql_int_unsigned,
        class_period,  // AA::Student::m_classPeriod (std::size_t)
        mysqlpp::sql_varchar,
        school_name,  // Foreign key -> schools.name (AA::Student::m_school)
        mysqlpp::sql_text,
        tests_json  // AA::Student::m_tests (Serialized JSON vector)
        )

    // 4. Tests table (maps to AA::Test metadata)
    sql_create_8(
        tests, 1, 8, mysqlpp::sql_varchar,
        test_id,                     // AA::Test::m_testID (Primary Key)
        mysqlpp::sql_varchar, name,  // AA::Test::m_name
        mysqlpp::sql_varchar,
        teacher_id,  // Foreign key -> teachers.teacher_id (AA::Test::m_teacher)
        mysqlpp::sql_varchar,
        sent_date,  // AA::Test::m_sentDate (DT::Date::toStringSql())
        mysqlpp::sql_varchar,
        due_date,  // AA::Test::m_dueDate (DT::Date::toStringSql())
        mysqlpp::sql_varchar, subject,    // AA::Test::m_subject
        mysqlpp::sql_text, description,   // AA::Test::m_description
        mysqlpp::sql_int_unsigned, level  // AA::Test::m_level (std::size_t)
        )

    // 5. Test questions table (maps to AA::Test::m_test / AA::QandA)
    sql_create_4(test_questions, 1, 4, mysqlpp::sql_int,
                 id,                             // Auto-increment Primary Key
                 mysqlpp::sql_varchar, test_id,  // Foreign key -> tests.test_id
                 mysqlpp::sql_text, question,    // AA::QandA::m_question
                 mysqlpp::sql_text, answer       // AA::QandA::m_answer
                 )

    // 6. Student test results table (maps to AA::Test::m_students / studentExt)
    sql_create_7(
        student_tests, 1, 7, mysqlpp::sql_int,
        id,                                // Auto-increment Primary Key
        mysqlpp::sql_varchar, test_id,     // Foreign key -> tests.test_id
        mysqlpp::sql_varchar, student_id,  // Foreign key -> students.student_id
        mysqlpp::sql_text,
        return_test,  // studentExt::m_returnTest (serialized JSON array)
        mysqlpp::sql_text,
        wrong_answers,  // studentExt::m_wrongAnswers (serialized JSON array)
        mysqlpp::sql_float, grade,  // studentExt::m_grade
        mysqlpp::sql_varchar,
        rcv_date  // studentExt::m_rcvDate (DT::Date::toStringSql())
        )

    // 7. Teacher roster table (maps a Teacher -> Students and their Class
    // Period)
    sql_create_complete_4(teacher_roster, 1, 4, mysqlpp::sql_int, id,
                          "id",  // Auto-increment Primary Key
                          mysqlpp::sql_varchar, teacher_id,
                          "teacher_id",  // Foreign key -> teachers.teacher_id
                          mysqlpp::sql_varchar, student_id,
                          "student_id",  // Foreign key -> students.student_id
                          mysqlpp::sql_int_unsigned, class_period,
                          "class_period"  // Class period student is in
                          )

    // 8. Student schedule view (maps a Student -> Teachers and the Class Period
    // they have them)
    sql_create_complete_4(
        student_schedule, 1, 4, mysqlpp::sql_int, id,
        "id",  // Row ID from teacher_roster view
        mysqlpp::sql_varchar, student_id,
        "student_id",  // Foreign key -> students.student_id
        mysqlpp::sql_varchar, teacher_id,
        "teacher_id",  // Foreign key -> teachers.teacher_id
        mysqlpp::sql_int_unsigned, class_period,
        "class_period"  // Class period student has that teacher
        )

    // 9. Open tests table (maps to active/assigned tests in OpenTestSingleton)
    sql_create_6(open_tests, 1, 6, mysqlpp::sql_int,
                 id,                             // Auto-increment Primary Key
                 mysqlpp::sql_varchar, test_id,  // Foreign key -> tests.test_id
                 mysqlpp::sql_varchar,
                 teacher_id,  // Foreign key -> teachers.teacher_id (teacher who
                              // gave test)
                 mysqlpp::sql_varchar,
                 creation_date,                   // DT::Date::toStringSql()
                 mysqlpp::sql_varchar, due_date,  // DT::Date::toStringSql()
                 mysqlpp::sql_int_unsigned,
                 duration  // Duration the test is supposed to last
                 )

    // 10. Student open test log (tracks assignments, completion status,
    // submission date, and scores)
    sql_create_7(
        student_open_test_log, 1, 7, mysqlpp::sql_int,
        id,                                // Auto-increment Primary Key
        mysqlpp::sql_varchar, student_id,  // Foreign key -> students.id
        mysqlpp::sql_int, open_test_id,    // Foreign key -> open_tests.id
        mysqlpp::sql_varchar, status,  // 'Pending', 'Submitted', 'Graded', etc.
        mysqlpp::sql_varchar, submission_date,  // Stores DT::Date string format
        mysqlpp::sql_double, score,             // Test score / grade
        mysqlpp::sql_int_unsigned,
        class_period  // Student's period for this open test
        )

        sql_create_3(
            gradebooks, 1, 3, mysqlpp::sql_varchar,
            name,  // Gradebook identifier/name (Primary Key)
            mysqlpp::sql_varchar,
            url,  // Gradebook API or submission endpoint URL
            mysqlpp::sql_text,
            post_temp_script  // POST template script / payload template
        )

}  // namespace SSQLS

#pragma GCC diagnostic pop

// Clean up macro so it never leaks into headers included after SSQLS.h
#undef MYSQLPP_SSQLS_NO_STATICS

#endif  // SSQLS_H
