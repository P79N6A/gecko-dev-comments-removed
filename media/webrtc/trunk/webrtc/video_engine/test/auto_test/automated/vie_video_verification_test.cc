









#include <sstream>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/metrics/video_metrics.h"
#include "webrtc/test/testsupport/perf_test.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_file_based_comparison_tests.h"
#include "webrtc/video_engine/test/auto_test/primitives/framedrop_primitives.h"
#include "webrtc/video_engine/test/libvietest/include/tb_external_transport.h"
#include "webrtc/video_engine/test/libvietest/include/vie_to_file_renderer.h"

namespace {




const int kInputWidth = 176;
const int kInputHeight = 144;

class ViEVideoVerificationTest : public testing::Test {
 protected:
  void SetUp() {
    input_file_ = webrtc::test::ResourcePath("paris_qcif", "yuv");
    local_file_renderer_ = NULL;
    remote_file_renderer_ = NULL;
  }

  void InitializeFileRenderers() {
    local_file_renderer_ = new ViEToFileRenderer();
    remote_file_renderer_ = new ViEToFileRenderer();
    SetUpLocalFileRenderer(local_file_renderer_);
    SetUpRemoteFileRenderer(remote_file_renderer_);
  }

  void SetUpLocalFileRenderer(ViEToFileRenderer* file_renderer) {
    SetUpFileRenderer(file_renderer, "-local-preview.yuv");
  }

  void SetUpRemoteFileRenderer(ViEToFileRenderer* file_renderer) {
    SetUpFileRenderer(file_renderer, "-remote.yuv");
  }

  
  void StopRenderers() {
    local_file_renderer_->StopRendering();
    remote_file_renderer_->StopRendering();
  }

  void CompareFiles(const std::string& reference_file,
                    const std::string& test_file,
                    double* psnr_result, double *ssim_result) {
    webrtc::test::QualityMetricsResult psnr;
    int error = I420PSNRFromFiles(reference_file.c_str(), test_file.c_str(),
                                  kInputWidth, kInputHeight, &psnr);

    EXPECT_EQ(0, error) << "PSNR routine failed - output files missing?";
    *psnr_result = psnr.average;

    webrtc::test::QualityMetricsResult ssim;
    error = I420SSIMFromFiles(reference_file.c_str(), test_file.c_str(),
                              kInputWidth, kInputHeight, &ssim);
    EXPECT_EQ(0, error) << "SSIM routine failed - output files missing?";
    *ssim_result = ssim.average;

    ViETest::Log("Results: PSNR is %f (dB; 48 is max), "
                 "SSIM is %f (1 is perfect)",
                 psnr.average, ssim.average);
  }

  
  void TearDownFileRenderers() {
    TearDownFileRenderer(local_file_renderer_);
    TearDownFileRenderer(remote_file_renderer_);
  }

  std::string input_file_;
  ViEToFileRenderer* local_file_renderer_;
  ViEToFileRenderer* remote_file_renderer_;
  ViEFileBasedComparisonTests tests_;

 private:
  void SetUpFileRenderer(ViEToFileRenderer* file_renderer,
                         const std::string& suffix) {
    std::string output_path = ViETest::GetResultOutputPath();
    std::string filename = "render_output" + suffix;

    if (!file_renderer->PrepareForRendering(output_path, filename)) {
      FAIL() << "Could not open output file " << filename <<
          " for writing.";
    }
  }

  void TearDownFileRenderer(ViEToFileRenderer* file_renderer) {
      assert(file_renderer);
      bool test_failed = ::testing::UnitTest::GetInstance()->
          current_test_info()->result()->Failed();
      if (test_failed) {
        
        file_renderer->SaveOutputFile("failed-");
      }
      delete file_renderer;
    }
};

TEST_F(ViEVideoVerificationTest, RunsBaseStandardTestWithoutErrors) {
  
  
  
  
  
  
  const double kReasonablePsnr = webrtc::test::kMetricsPerfectPSNR - 2.0f;
  const double kReasonableSsim = 0.99f;
  const int kNumAttempts = 5;
  for (int attempt = 0; attempt < kNumAttempts; ++attempt) {
    InitializeFileRenderers();
    ASSERT_TRUE(tests_.TestCallSetup(input_file_, kInputWidth, kInputHeight,
                                     local_file_renderer_,
                                     remote_file_renderer_));
    std::string remote_file = remote_file_renderer_->GetFullOutputPath();
    std::string local_preview = local_file_renderer_->GetFullOutputPath();
    StopRenderers();

    double actual_psnr = 0;
    double actual_ssim = 0;
    CompareFiles(local_preview, remote_file, &actual_psnr, &actual_ssim);

    TearDownFileRenderers();

    if (actual_psnr > kReasonablePsnr && actual_ssim > kReasonableSsim) {
      
      return;
    } else {
      ViETest::Log("Retrying; attempt %d of %d.", attempt + 1, kNumAttempts);
    }
  }

  FAIL() << "Failed to achieve near-perfect PSNR and SSIM results after " <<
      kNumAttempts << " attempts.";
}

}  
