









#include "gtest/gtest.h"

int RunInAutomatedMode(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
