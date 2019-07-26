









#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/video_engine/test/common/flags.h"
#include "webrtc/video_engine/test/common/run_tests.h"

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  webrtc::test::flags::Init(&argc, &argv);
  webrtc::test::SetExecutablePath(argv[0]);

  return webrtc::test::RunAllTests();
}
