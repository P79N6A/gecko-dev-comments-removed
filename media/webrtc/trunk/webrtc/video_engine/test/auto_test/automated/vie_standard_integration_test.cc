














#include <stdio.h>

#include "gflags/gflags.h"
#include "testing/gtest/include/gtest/gtest.h"
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


TEST_F(ViEStandardIntegrationTest, DISABLED_RunsCodecTestWithoutErrors)  {
  tests_->ViECodecStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsCaptureTestWithoutErrors)  {
  tests_->ViECaptureStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsImageProcessTestWithoutErrors)  {
  tests_->ViEImageProcessStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsRenderTestWithoutErrors)  {
  tests_->ViERenderStandardTest();
}


#if defined(WEBRTC_MAC)
#define MAYBE_RunsRtpRtcpTestWithoutErrors DISABLED_RunsRtpRtcpTestWithoutErrors
#else
#define MAYBE_RunsRtpRtcpTestWithoutErrors RunsRtpRtcpTestWithoutErrors
#endif
TEST_F(ViEStandardIntegrationTest, MAYBE_RunsRtpRtcpTestWithoutErrors)  {
  tests_->ViERtpRtcpStandardTest();
}

}  
