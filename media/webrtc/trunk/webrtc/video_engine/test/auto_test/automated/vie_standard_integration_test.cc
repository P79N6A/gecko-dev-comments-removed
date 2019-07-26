
















#include <cstdio>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "legacy_fixture.h"
#include "testsupport/metrics/video_metrics.h"
#include "vie_autotest.h"
#include "vie_autotest_window_manager_interface.h"
#include "vie_to_file_renderer.h"
#include "vie_window_creator.h"
#include "testsupport/metrics/video_metrics.h"

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

TEST_F(ViEStandardIntegrationTest, RunsNetworkTestWithoutErrors)  {
  tests_->ViENetworkStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsRenderTestWithoutErrors)  {
  tests_->ViERenderStandardTest();
}

TEST_F(ViEStandardIntegrationTest, RunsRtpRtcpTestWithoutErrors)  {
  tests_->ViERtpRtcpStandardTest();
}

} 
