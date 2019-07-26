













#include "gtest/gtest.h"
#include "legacy_fixture.h"
#include "vie_autotest.h"

namespace {

class ViEApiIntegrationTest : public LegacyFixture {
};

TEST_F(ViEApiIntegrationTest, RunsBaseTestWithoutErrors) {
  tests_->ViEBaseAPITest();
}


TEST_F(ViEApiIntegrationTest, DISABLED_RunsCaptureTestWithoutErrors) {
  tests_->ViECaptureAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsCodecTestWithoutErrors) {
  tests_->ViECodecAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsEncryptionTestWithoutErrors) {
  tests_->ViEEncryptionAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsFileTestWithoutErrors) {
  tests_->ViEFileAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsImageProcessTestWithoutErrors) {
  tests_->ViEImageProcessAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsNetworkTestWithoutErrors) {
  tests_->ViENetworkAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsRenderTestWithoutErrors) {
  tests_->ViERenderAPITest();
}

TEST_F(ViEApiIntegrationTest, RunsRtpRtcpTestWithoutErrors) {
  tests_->ViERtpRtcpAPITest();
}

} 
