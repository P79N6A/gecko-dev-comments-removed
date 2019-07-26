









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/video_engine/test/auto_test/automated/legacy_fixture.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"

namespace {



class DISABLED_ON_MAC(ViEApiIntegrationTest) : public LegacyFixture {
};

TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest), RunsBaseTestWithoutErrors) {
  tests_->ViEBaseAPITest();
}


TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest),
       DISABLED_RunsCaptureTestWithoutErrors) {
  tests_->ViECaptureAPITest();
}

TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest), RunsCodecTestWithoutErrors) {
  tests_->ViECodecAPITest();
}

TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest),
       RunsImageProcessTestWithoutErrors) {
  tests_->ViEImageProcessAPITest();
}

TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest), RunsRenderTestWithoutErrors) {
  tests_->ViERenderAPITest();
}


TEST_F(DISABLED_ON_MAC(ViEApiIntegrationTest),
       DISABLED_RunsRtpRtcpTestWithoutErrors) {
  tests_->ViERtpRtcpAPITest();
}

}  
