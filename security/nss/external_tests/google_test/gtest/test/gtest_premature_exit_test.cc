

































#include <stdio.h>

#include "gtest/gtest.h"

using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::internal::posix::GetEnv;
using ::testing::internal::posix::Stat;
using ::testing::internal::posix::StatStruct;

namespace {



const bool kTestPrematureExitFileEnvVarShouldBeSet = false;

class PrematureExitTest : public Test {
 public:
  
  static bool FileExists(const char* filepath) {
    StatStruct stat;
    return Stat(filepath, &stat) == 0;
  }

 protected:
  PrematureExitTest() {
    premature_exit_file_path_ = GetEnv("TEST_PREMATURE_EXIT_FILE");

    
    if (premature_exit_file_path_ == NULL) {
      premature_exit_file_path_ = "";
    }
  }

  
  bool PrematureExitFileExists() const {
    return FileExists(premature_exit_file_path_);
  }

  const char* premature_exit_file_path_;
};

typedef PrematureExitTest PrematureExitDeathTest;






TEST_F(PrematureExitDeathTest, FileExistsDuringExecutionOfDeathTest) {
  if (*premature_exit_file_path_ == '\0') {
    return;
  }

  EXPECT_DEATH_IF_SUPPORTED({
      
      
      
      
      if (PrematureExitFileExists()) {
        exit(1);
      }
    }, "");
}



TEST_F(PrematureExitTest, TestPrematureExitFileEnvVarIsSet) {
  GTEST_INTENTIONAL_CONST_COND_PUSH_()
  if (kTestPrematureExitFileEnvVarShouldBeSet) {
  GTEST_INTENTIONAL_CONST_COND_POP_()
    const char* const filepath = GetEnv("TEST_PREMATURE_EXIT_FILE");
    ASSERT_TRUE(filepath != NULL);
    ASSERT_NE(*filepath, '\0');
  }
}



TEST_F(PrematureExitTest, PrematureExitFileExistsDuringTestExecution) {
  if (*premature_exit_file_path_ == '\0') {
    return;
  }

  EXPECT_TRUE(PrematureExitFileExists())
      << " file " << premature_exit_file_path_
      << " should exist during test execution, but doesn't.";
}

}  

int main(int argc, char **argv) {
  InitGoogleTest(&argc, argv);
  const int exit_code = RUN_ALL_TESTS();

  
  
  const char* const filepath = GetEnv("TEST_PREMATURE_EXIT_FILE");
  if (filepath != NULL && *filepath != '\0') {
    if (PrematureExitTest::FileExists(filepath)) {
      printf(
          "File %s shouldn't exist after the test program finishes, but does.",
          filepath);
      return 1;
    }
  }

  return exit_code;
}
