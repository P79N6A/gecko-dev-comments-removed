









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/video_engine/test/auto_test/automated/legacy_fixture.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"

namespace {



class DISABLED_ON_MAC(ViEExtendedIntegrationTest) : public LegacyFixture {
};

TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest), RunsBaseTestWithoutErrors) {
  tests_->ViEBaseExtendedTest();
}


TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest),
       DISABLED_RunsCaptureTestWithoutErrors) {
  tests_->ViECaptureExtendedTest();
}



#if defined(_WIN32)
#define MAYBE_RunsCodecTestWithoutErrors DISABLED_RunsCodecTestWithoutErrors
#else
#define MAYBE_RunsCodecTestWithoutErrors RunsCodecTestWithoutErrors
#endif
TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest),
       MAYBE_RunsCodecTestWithoutErrors) {
  tests_->ViECodecExtendedTest();
}

TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest),
       RunsImageProcessTestWithoutErrors) {
  tests_->ViEImageProcessExtendedTest();
}

TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest),
       RunsRenderTestWithoutErrors) {
  tests_->ViERenderExtendedTest();
}

TEST_F(DISABLED_ON_MAC(ViEExtendedIntegrationTest),
       RunsRtpRtcpTestWithoutErrors) {
  tests_->ViERtpRtcpExtendedTest();
}

}  
