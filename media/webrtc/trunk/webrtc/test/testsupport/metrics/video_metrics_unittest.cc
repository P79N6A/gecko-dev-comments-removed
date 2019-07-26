









#include "webrtc/test/testsupport/metrics/video_metrics.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

static const char* kNonExistingFileName = "video_metrics_unittest_non_existing";
static const int kWidth = 352;
static const int kHeight = 288;

static const int kMissingReferenceFileReturnCode = -1;
static const int kMissingTestFileReturnCode = -2;
static const int kEmptyFileReturnCode = -3;
static const double kPsnrPerfectResult =  48.0;
static const double kSsimPerfectResult = 1.0;

class VideoMetricsTest: public testing::Test {
 protected:
  VideoMetricsTest() {
    empty_file_ = webrtc::test::OutputPath() +
        "video_metrics_unittest_empty_file.tmp";
    video_file_ = webrtc::test::ResourcePath("foreman_cif_short", "yuv");
  }
  virtual ~VideoMetricsTest() {}
  void SetUp() {
    
    FILE* dummy = fopen(empty_file_.c_str(), "wb");
    fclose(dummy);
  }
  void TearDown() {
    remove(empty_file_.c_str());
  }
  webrtc::test::QualityMetricsResult psnr_result_;
  webrtc::test::QualityMetricsResult ssim_result_;
  std::string empty_file_;
  std::string video_file_;
};


TEST_F(VideoMetricsTest, ReturnsPerfectResultForIdenticalFilesPSNR) {
  EXPECT_EQ(0, I420PSNRFromFiles(video_file_.c_str(), video_file_.c_str(),
                                 kWidth, kHeight, &psnr_result_));
  EXPECT_EQ(kPsnrPerfectResult, psnr_result_.average);
}

TEST_F(VideoMetricsTest, ReturnsPerfectResultForIdenticalFilesSSIM) {
  EXPECT_EQ(0, I420SSIMFromFiles(video_file_.c_str(), video_file_.c_str(),
                                 kWidth, kHeight, &ssim_result_));
  EXPECT_EQ(kSsimPerfectResult, ssim_result_.average);
}

TEST_F(VideoMetricsTest, ReturnsPerfectResultForIdenticalFilesBothMetrics) {
  EXPECT_EQ(0, I420MetricsFromFiles(video_file_.c_str(), video_file_.c_str(),
                                    kWidth, kHeight, &psnr_result_,
                                    &ssim_result_));
  EXPECT_EQ(kPsnrPerfectResult, psnr_result_.average);
  EXPECT_EQ(kSsimPerfectResult, ssim_result_.average);
}


TEST_F(VideoMetricsTest, MissingReferenceFilePSNR) {
  EXPECT_EQ(kMissingReferenceFileReturnCode,
            I420PSNRFromFiles(kNonExistingFileName, video_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, MissingReferenceFileSSIM) {
  EXPECT_EQ(kMissingReferenceFileReturnCode,
            I420SSIMFromFiles(kNonExistingFileName, video_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, MissingReferenceFileBothMetrics) {
  EXPECT_EQ(kMissingReferenceFileReturnCode,
            I420MetricsFromFiles(kNonExistingFileName, video_file_.c_str(),
                                 kWidth, kHeight,
                                 &psnr_result_, &ssim_result_));
}


TEST_F(VideoMetricsTest, MissingTestFilePSNR) {
  EXPECT_EQ(kMissingTestFileReturnCode,
            I420PSNRFromFiles(video_file_.c_str(), kNonExistingFileName,
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, MissingTestFileSSIM) {
  EXPECT_EQ(kMissingTestFileReturnCode,
            I420SSIMFromFiles(video_file_.c_str(), kNonExistingFileName,
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, MissingTestFileBothMetrics) {
  EXPECT_EQ(kMissingTestFileReturnCode,
            I420MetricsFromFiles(video_file_.c_str(), kNonExistingFileName,
                                 kWidth, kHeight,
                                 &psnr_result_, &ssim_result_));
}


TEST_F(VideoMetricsTest, EmptyFilesPSNR) {
  EXPECT_EQ(kEmptyFileReturnCode,
            I420PSNRFromFiles(empty_file_.c_str(), video_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
  EXPECT_EQ(kEmptyFileReturnCode,
            I420PSNRFromFiles(video_file_.c_str(), empty_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, EmptyFilesSSIM) {
  EXPECT_EQ(kEmptyFileReturnCode,
            I420SSIMFromFiles(empty_file_.c_str(), video_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
  EXPECT_EQ(kEmptyFileReturnCode,
            I420SSIMFromFiles(video_file_.c_str(), empty_file_.c_str(),
                              kWidth, kHeight, &ssim_result_));
}

TEST_F(VideoMetricsTest, EmptyFilesBothMetrics) {
  EXPECT_EQ(kEmptyFileReturnCode,
            I420MetricsFromFiles(empty_file_.c_str(), video_file_.c_str(),
                                 kWidth, kHeight,
                                 &psnr_result_, &ssim_result_));
  EXPECT_EQ(kEmptyFileReturnCode,
              I420MetricsFromFiles(video_file_.c_str(), empty_file_.c_str(),
                                   kWidth, kHeight,
                                   &psnr_result_, &ssim_result_));
}

}  
