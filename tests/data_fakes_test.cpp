#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "Gradebook.h"
#include "OpenTest.h"
#include "School.h"
#include "Teacher.h"
#include "Test.h"
#include "fakes/FakeDataBase.h"
#include "fakes/FakeGradebook.h"
#include "fakes/FakeSchool.h"
#include "fakes/FakeTeacher.h"

namespace {

struct DummyRecord {
  int id = 0;
  std::string label;
  bool operator==(const DummyRecord& other) const = default;
};

// Adjust these setters/getters if your AA:: domain classes use different names.
AA::Gradebook makeGradebook(const std::string& name, const std::string& url,
                            const std::string& script = "post_grades()") {
  AA::Gradebook gb;
  gb.setName(name);
  gb.setURL(url);
  gb.setPostTemp(script);
  return gb;
}

AA::School makeSchool(const std::string& name, const std::string& address,
                      const std::string& gradebook = "Canvas") {
  AA::School s;
  s.setName(name);
  s.setAddress(address);
  s.setGradebook(gradebook);
  return s;
}

AA::Teacher makeTeacher(const std::string& id, const std::string& firstName,
                        const std::string& lastName, const std::string& email,
                        const std::string& schoolName) {
  AA::School s{};
  s.setName(schoolName);
  AA::Teacher t;
  t.setTeacherID(id);
  t.setFName(firstName);
  t.setLName(lastName);
  t.setEmailAddr(email);
  t.setSchool(s);
  return t;
}

class AutoAssignmentFakesSuite : public ::testing::Test {
 protected:
  AA::Data::Fakes::gradebook gradebookStore;
  AA::Data::Fakes::school schoolStore;
  AA::Data::Fakes::teacher teacherStore;

  void SetUp() override {
    gradebookStore.clear();
    schoolStore.clear();
    teacherStore.clear();
  }
};

// --- FakeDataBase Direct Contract Tests ---

TEST(FakeDataBaseTest, CrudContractWithIntegerKey) {
  AA::Data::Fakes::FakeDataBase<DummyRecord, int> store(
      "dummy_records", [](const DummyRecord& r) { return r.id; });

  EXPECT_EQ(store.getTableName(), "dummy_records");
  EXPECT_EQ(store.size(), 0u);

  const DummyRecord rec1{1, "first"};
  EXPECT_TRUE(store.save(rec1));
  EXPECT_EQ(store.size(), 1u);

  // Duplicate ID should fail and keep original value
  EXPECT_FALSE(store.save(DummyRecord{1, "duplicate"}));

  DummyRecord out;
  ASSERT_TRUE(store.read("id", 1, out));
  EXPECT_EQ(out, rec1);

  // Update existing record
  EXPECT_TRUE(store.update(DummyRecord{1, "updated"}, "id", 1));
  ASSERT_TRUE(store.read("id", 1, out));
  EXPECT_EQ(out.label, "updated");

  // Updating, reading, or removing a missing ID returns false
  EXPECT_FALSE(store.update(DummyRecord{99, "missing"}, "id", 99));
  EXPECT_FALSE(store.read("id", 99, out));
  EXPECT_FALSE(store.remove("id", 99));

  // Remove existing record
  EXPECT_TRUE(store.remove("id", 1));
  EXPECT_EQ(store.size(), 0u);
  EXPECT_FALSE(store.read("id", 1, out));
}

// --- Gradebook CRUD Tests ---

TEST_F(AutoAssignmentFakesSuite, GradebookSaveReadUpdateRemove) {
  AA::Gradebook gb = makeGradebook("PowerSchool", "https://ps.example.edu",
                                   "syncPowerSchool()");

  EXPECT_EQ(gradebookStore.getTableName(), "gradebooks");
  EXPECT_TRUE(gradebookStore.save(gb));
  EXPECT_FALSE(gradebookStore.save(gb));  // Duplicate primary key rejected

  AA::Gradebook loaded;
  ASSERT_TRUE(gradebookStore.read("name", "PowerSchool", loaded));
  EXPECT_EQ(loaded.getName(), "PowerSchool");
  EXPECT_EQ(loaded.getURL(), "https://ps.example.edu");

  AA::Gradebook updated =
      makeGradebook("PowerSchool", "https://ps-v2.example.edu", "syncV2()");
  EXPECT_TRUE(gradebookStore.update(updated, "name", "PowerSchool"));

  AA::Gradebook reloaded;
  ASSERT_TRUE(gradebookStore.read("name", "PowerSchool", reloaded));
  EXPECT_EQ(reloaded.getURL(), "https://ps-v2.example.edu");

  EXPECT_TRUE(gradebookStore.remove("name", "PowerSchool"));
  EXPECT_FALSE(gradebookStore.remove("name", "PowerSchool"));

  AA::Gradebook missing;
  EXPECT_FALSE(gradebookStore.read("name", "PowerSchool", missing));
}

// --- School CRUD Tests ---

TEST_F(AutoAssignmentFakesSuite, SchoolSaveReadUpdateRemove) {
  AA::School s =
      makeSchool("Oakridge High", "100 Oak St, Springfield", "PowerSchool");

  EXPECT_EQ(schoolStore.getTableName(), "schools");
  EXPECT_TRUE(schoolStore.save(s));
  EXPECT_FALSE(schoolStore.save(s));  // Duplicate school name rejected

  AA::School loaded;
  ASSERT_TRUE(schoolStore.read("name", "Oakridge High", loaded));
  EXPECT_EQ(loaded.getName(), "Oakridge High");
  EXPECT_EQ(loaded.getAddress(), "100 Oak St, Springfield");

  AA::School updated =
      makeSchool("Oakridge High", "200 Maple Ave, Springfield", "Canvas");
  EXPECT_TRUE(schoolStore.update(updated, "name", "Oakridge High"));

  AA::School reloaded;
  ASSERT_TRUE(schoolStore.read("name", "Oakridge High", reloaded));
  EXPECT_EQ(reloaded.getAddress(), "200 Maple Ave, Springfield");
  EXPECT_EQ(reloaded.getGradebook(), "Canvas");

  EXPECT_TRUE(schoolStore.remove("name", "Oakridge High"));
  AA::School missing;
  EXPECT_FALSE(schoolStore.read("name", "Oakridge High", missing));
}

// --- Teacher CRUD & Domain Operations Tests ---

TEST_F(AutoAssignmentFakesSuite, TeacherSaveReadUpdateRemove) {
  AA::Teacher t = makeTeacher("TCH_001", "Ada", "Lovelace", "ada@oakridge.edu",
                              "Oakridge High");

  EXPECT_EQ(teacherStore.getTableName(), "teachers");
  EXPECT_TRUE(teacherStore.save(t));
  EXPECT_FALSE(teacherStore.save(t));  // Duplicate teacher_id rejected

  AA::Teacher loaded;
  ASSERT_TRUE(teacherStore.read("teacher_id", "TCH_001", loaded));
  EXPECT_EQ(loaded.getTeacherID(), "TCH_001");
  EXPECT_EQ(loaded.getEmailAddr(), "ada@oakridge.edu");

  AA::Teacher updated = makeTeacher("TCH_001", "Ada", "Byron",
                                    "ada.byron@oakridge.edu", "Oakridge High");
  EXPECT_TRUE(teacherStore.update(updated, "teacher_id", "TCH_001"));

  AA::Teacher reloaded;
  ASSERT_TRUE(teacherStore.read("teacher_id", "TCH_001", reloaded));
  EXPECT_EQ(reloaded.getLName(), "Byron");
  EXPECT_EQ(reloaded.getEmailAddr(), "ada.byron@oakridge.edu");

  EXPECT_TRUE(teacherStore.remove("teacher_id", "TCH_001"));
  AA::Teacher missing;
  EXPECT_FALSE(teacherStore.read("teacher_id", "TCH_001", missing));
}

TEST_F(AutoAssignmentFakesSuite, TeacherCredentialsAndRosterWorkflow) {
  EXPECT_FALSE(teacherStore.saveCredentials("", "salt", "hash"));
  EXPECT_TRUE(
      teacherStore.saveCredentials("TCH_001", "random_salt", "hashed_pw"));

  std::string salt;
  std::string password;
  ASSERT_TRUE(teacherStore.loadCredentials("TCH_001", salt, password));
  EXPECT_EQ(salt, "random_salt");
  EXPECT_EQ(password, "hashed_pw");

  EXPECT_FALSE(teacherStore.loadCredentials("UNKNOWN_ID", salt, password));

  const std::vector<AA::Data::Fakes::teacher::RosterEntry> roster = {
      {"STU_10", 1u}, {"STU_11", 1u}, {"STU_12", 3u}};
  EXPECT_FALSE(teacherStore.saveTeacherRoster("", roster));
  EXPECT_TRUE(teacherStore.saveTeacherRoster("TCH_001", roster));

  std::vector<AA::Data::Fakes::teacher::RosterEntry> loadedRoster;
  ASSERT_TRUE(teacherStore.loadTeacherRoster("TCH_001", loadedRoster));
  ASSERT_EQ(loadedRoster.size(), 3u);
  EXPECT_EQ(loadedRoster[0], std::make_pair(std::string("STU_10"), 1u));
  EXPECT_EQ(loadedRoster[2], std::make_pair(std::string("STU_12"), 3u));
}

TEST_F(AutoAssignmentFakesSuite, TeacherSeededTestsAndOpenTestAssignments) {
  AA::Test test1;
  test1.setTestID("TST_101");
  test1.setTestName("Algebra Quiz 1");

  AA::Test test2;
  test2.setTestID("TST_102");
  test2.setTestName("Polynomials Midterm");

  teacherStore.seedTests("TCH_001", {test1, test2});

  std::vector<AA::Test> tests;
  ASSERT_TRUE(teacherStore.loadTestsForTeacher("TCH_001", tests));
  ASSERT_EQ(tests.size(), 2u);
  EXPECT_EQ(tests[0].getTestID(), "TST_101");
  EXPECT_EQ(tests[1].getTestName(), "Polynomials Midterm");

  EXPECT_FALSE(teacherStore.loadTestsForTeacher("TCH_999", tests));

  AA::OpenTest openTest;
  openTest.setTestID("TST_101");
  teacherStore.seedOpenTests("TCH_001", {openTest});

  std::vector<AA::OpenTest> openTests;
  ASSERT_TRUE(teacherStore.loadOpenTestsForTeacher("TCH_001", openTests));
  ASSERT_EQ(openTests.size(), 1u);
  EXPECT_EQ(openTests[0].getTestID(), "TST_101");

  // Invalid inputs should fail cleanly
  EXPECT_FALSE(teacherStore.assignOpenTestToMultipleStudents({}, 50));
  EXPECT_FALSE(teacherStore.assignOpenTestToMultipleStudents({"STU_10"}, 0));

  // Valid assignment
  EXPECT_TRUE(
      teacherStore.assignOpenTestToMultipleStudents({"STU_10", "STU_11"}, 50));
  const auto& assigned = teacherStore.assignedStudentsForOpenTest(50);
  ASSERT_EQ(assigned.size(), 2u);
  EXPECT_EQ(assigned[0], "STU_10");
  EXPECT_EQ(assigned[1], "STU_11");
}

}  // namespace
