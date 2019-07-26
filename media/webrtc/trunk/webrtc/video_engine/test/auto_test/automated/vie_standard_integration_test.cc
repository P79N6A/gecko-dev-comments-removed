














#include <cstdio>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "webrtc/test/testsupport/metrics/video_metrics.h"
#include "webrtc/test/testsupport/metrics/video_metrics.h"
#include "webrtc/video_engine/test/auto_test/automated/legacy_fixture.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_window_manager_interface.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_window_creator.h"
#include "webrtc/video_engine/test/libvietest/include/vie_to_file_renderer.h"

namespace {

class ViEStandardIntegrationTest : public LegacyFixture {
};

TEST_F(ViEStandardIntegrationTest, RunsBaseTestWithoutErrors)  {
  tests_->ViEBaseStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsCodecTestWithoutErrors)  {
  tests_->ViECodecStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsCaptureTestWithoutErrors)  {
  tests_->ViECaptureStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsEncryptionTestWithoutErrors)  {
  tests_->ViEEncryptionStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsFileTestWithoutErrors)  {
  tests_->ViEFileStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsImageProcessTestWithoutErrors)  {
  tests_->ViEImageProcessStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsRenderTestWithoutErrors)  {
  tests_->ViERenderStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsRtpRtcpTestWithoutErrors)  {
  tests_->ViERtpRtcpStandardTest();
}

} 
