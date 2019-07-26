









#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/test/flags.h"
#include "webrtc/test/run_tests.h"
#include "webrtc/test/testsupport/fileutils.h"

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  webrtc::test::flags::Init(&argc, &argv);
  webrtc::test::SetExecutablePath(argv[0]);

  return webrtc::test::RunAllTests();
}
