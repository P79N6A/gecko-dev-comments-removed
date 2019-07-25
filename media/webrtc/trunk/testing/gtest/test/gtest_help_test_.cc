

































#include "gtest/gtest.h"




TEST(HelpFlagTest, ShouldNotBeRun) {
  ASSERT_TRUE(false) << "Tests shouldn't be run when --help is specified.";
}

#if GTEST_HAS_DEATH_TEST
TEST(DeathTest, UsedByPythonScriptToDetectSupportForDeathTestsInThisBinary) {}
#endif
