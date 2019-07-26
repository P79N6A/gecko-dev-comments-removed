









#include "gtest/gtest.h"
#include "test/testsupport/fileutils.h"

void InitializeGoogleTest(int* argc, char** argv) {
  
  webrtc::test::SetExecutablePath(argv[0]);
  testing::InitGoogleTest(argc, argv);
}

int RunInAutomatedMode() {
  return RUN_ALL_TESTS();
}
