













#include "gtest/gtest.h"
#include "legacy_fixture.h"
#include "vie_autotest.h"

namespace {

class ViEExtendedIntegrationTest : public LegacyFixture {
};

TEST_F(ViEExtendedIntegrationTest, RunsBaseTestWithoutErrors) {
  tests_->ViEBaseExtendedTest();
}


TEST_F(ViEExtendedIntegrationTest, DISABLED_RunsCaptureTestWithoutErrors) {
  tests_->ViECaptureExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsCodecTestWithoutErrors) {
  tests_->ViECodecExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsEncryptionTestWithoutErrors) {
  tests_->ViEEncryptionExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsFileTestWithoutErrors) {
  tests_->ViEFileExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsImageProcessTestWithoutErrors) {
  tests_->ViEImageProcessExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsNetworkTestWithoutErrors) {
  tests_->ViENetworkExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsRenderTestWithoutErrors) {
  tests_->ViERenderExtendedTest();
}

TEST_F(ViEExtendedIntegrationTest, RunsRtpRtcpTestWithoutErrors) {
  tests_->ViERtpRtcpExtendedTest();
}

} 
