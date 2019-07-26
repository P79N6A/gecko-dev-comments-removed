









#include <vector>

#include "gtest/gtest.h"
#include "testsupport/fileutils.h"
#include "testsupport/metrics/video_metrics.h"
#include "video_engine/test/auto_test/interface/vie_autotest.h"
#include "video_engine/test/auto_test/interface/vie_file_based_comparison_tests.h"
#include "video_engine/test/auto_test/primitives/framedrop_primitives.h"
#include "video_engine/test/libvietest/include/vie_to_file_renderer.h"

namespace {




const int kInputWidth = 176;
const int kInputHeight = 144;
const int kVerifyingTestMaxNumAttempts = 3;

class ViEVideoVerificationTest : public testing::Test {
 protected:
  void SetUp() {
    input_file_ = webrtc::test::ResourcePath("paris_qcif", "yuv");
  }

  void TearDown() {
    TearDownFileRenderer(local_file_renderer_);
    TearDownFileRenderer(remote_file_renderer_);
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

  void TearDownFileRenderer(ViEToFileRenderer* file_renderer) {
    assert(file_renderer);
    bool test_failed = ::testing::UnitTest::GetInstance()->
        current_test_info()->result()->Failed();
    if (test_failed) {
      
      file_renderer->SaveOutputFile("failed-");
    } else {
      
      file_renderer->DeleteOutputFile();
    }
    delete file_renderer;
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

    ViETest::Log("Results: PSNR is %f (dB), SSIM is %f (1 is perfect)",
                 psnr.average, ssim.average);
  }

  std::string input_file_;
  ViEToFileRenderer* local_file_renderer_;
  ViEToFileRenderer* remote_file_renderer_;
  ViEFileBasedComparisonTests tests_;

 private:
  void SetUpFileRenderer(ViEToFileRenderer* file_renderer,
                         const std::string& suffix) {
    std::string test_case_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();

    std::string output_path = ViETest::GetResultOutputPath();
    std::string filename = test_case_name + suffix;

    if (!file_renderer->PrepareForRendering(output_path, filename)) {
      FAIL() << "Could not open output file " << filename <<
          " for writing.";
    }
  }
};

TEST_F(ViEVideoVerificationTest, RunsBaseStandardTestWithoutErrors)  {
  
  
  
  
  const double kExpectedMinimumPSNR = 30;
  const double kExpectedMinimumSSIM = 0.95;

  for (int attempt = 0; attempt < kVerifyingTestMaxNumAttempts; attempt++) {
    InitializeFileRenderers();
    ASSERT_TRUE(tests_.TestCallSetup(input_file_, kInputWidth, kInputHeight,
                                     local_file_renderer_,
                                     remote_file_renderer_));
    std::string output_file = remote_file_renderer_->GetFullOutputPath();
    StopRenderers();

    double actual_psnr = 0;
    double actual_ssim = 0;
    CompareFiles(input_file_, output_file, &actual_psnr, &actual_ssim);

    if (actual_psnr >= kExpectedMinimumPSNR &&
        actual_ssim >= kExpectedMinimumSSIM) {
      
      return;
    }
  }

  ADD_FAILURE() << "Failed to achieve PSNR " << kExpectedMinimumPSNR <<
      " and SSIM " << kExpectedMinimumSSIM << " after " <<
      kVerifyingTestMaxNumAttempts << " attempts.";
}

TEST_F(ViEVideoVerificationTest, RunsCodecTestWithoutErrors)  {
  
  
  
  
  
  
  
  
  const double kExpectedMinimumPSNR = 20;
  const double kExpectedMinimumSSIM = 0.7;

  for (int attempt = 0; attempt < kVerifyingTestMaxNumAttempts; attempt++) {
    InitializeFileRenderers();
    ASSERT_TRUE(tests_.TestCodecs(input_file_, kInputWidth, kInputHeight,
                                  local_file_renderer_,
                                  remote_file_renderer_));
    std::string reference_file = local_file_renderer_->GetFullOutputPath();
    std::string output_file = remote_file_renderer_->GetFullOutputPath();
    StopRenderers();

    double actual_psnr = 0;
    double actual_ssim = 0;
    CompareFiles(reference_file, output_file, &actual_psnr, &actual_ssim);

    if (actual_psnr >= kExpectedMinimumPSNR &&
        actual_ssim >= kExpectedMinimumSSIM) {
      
      return;
    }
  }
}







TEST_F(ViEVideoVerificationTest, DISABLED_RunsFullStackWithoutErrors)  {
  FrameDropDetector detector;
  local_file_renderer_ = new ViEToFileRenderer();
  remote_file_renderer_ = new FrameDropMonitoringRemoteFileRenderer(&detector);
  SetUpLocalFileRenderer(local_file_renderer_);
  SetUpRemoteFileRenderer(remote_file_renderer_);

  
  
  const int kBitRateKbps = 50;
  const int kPacketLossPercent = 5;
  const int kNetworkDelayMs = 100;
  ViETest::Log("Bit rate     : %5d kbps", kBitRateKbps);
  ViETest::Log("Packet loss  : %5d %%", kPacketLossPercent);
  ViETest::Log("Network delay: %5d ms", kNetworkDelayMs);
  tests_.TestFullStack(input_file_, kInputWidth, kInputHeight, kBitRateKbps,
                       kPacketLossPercent, kNetworkDelayMs,
                       local_file_renderer_, remote_file_renderer_, &detector);
  const std::string reference_file = local_file_renderer_->GetFullOutputPath();
  const std::string output_file = remote_file_renderer_->GetFullOutputPath();
  StopRenderers();

  detector.CalculateResults();
  detector.PrintReport();

  if (detector.GetNumberOfFramesDroppedAt(FrameDropDetector::kRendered) !=
      detector.GetNumberOfFramesDroppedAt(FrameDropDetector::kDecoded)) {
    detector.PrintDebugDump();
  }

  ASSERT_EQ(detector.GetNumberOfFramesDroppedAt(FrameDropDetector::kRendered),
      detector.GetNumberOfFramesDroppedAt(FrameDropDetector::kDecoded))
      << "The number of dropped frames on the decode and render steps are not "
      "equal. This may be because we have a major problem in the buffers of "
      "the ViEToFileRenderer?";

  
  
  
  
  
  const int frame_length_in_bytes = 3 * kInputHeight * kInputWidth / 2;
  ViETest::Log("Frame length: %d bytes", frame_length_in_bytes);
  std::vector<Frame*> all_frames = detector.GetAllFrames();
  FixOutputFileForComparison(output_file, frame_length_in_bytes, all_frames);

  
  size_t output_file_size = webrtc::test::GetFileSize(output_file);
  EXPECT_EQ(all_frames.size(), output_file_size / frame_length_in_bytes)
      << "The output file size is incorrect. It should be equal to the number "
      "of frames multiplied by the frame size. This will likely affect "
      "PSNR/SSIM calculations in a bad way.";

  
  
  double actual_psnr = 0;
  double actual_ssim = 0;
  CompareFiles(reference_file, output_file, &actual_psnr, &actual_ssim);

  const double kExpectedMinimumPSNR = 24;
  const double kExpectedMinimumSSIM = 0.7;

  EXPECT_GE(actual_psnr, kExpectedMinimumPSNR);
  EXPECT_GE(actual_ssim, kExpectedMinimumSSIM);
}

}  
