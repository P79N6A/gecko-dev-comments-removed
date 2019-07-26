














#include <gtest/gtest.h>

#include "modules/video_coding/main/source/qm_select.h"
#include "modules/interface/module_common_types.h"

namespace webrtc {



const float kSpatialLow = 0.01f;
const float kSpatialMedium = 0.03f;
const float kSpatialHigh = 0.1f;
const float kTemporalLow = 0.01f;
const float kTemporalMedium = 0.06f;
const float kTemporalHigh = 0.1f;

class QmSelectTest : public ::testing::Test {
 protected:
  QmSelectTest()
      :  qm_resolution_(new VCMQmResolution()),
         content_metrics_(new VideoContentMetrics()),
         qm_scale_(NULL) {
  }
  VCMQmResolution* qm_resolution_;
  VideoContentMetrics* content_metrics_;
  VCMResolutionScale* qm_scale_;

  void InitQmNativeData(float initial_bit_rate,
                        int user_frame_rate,
                        int native_width,
                        int native_height,
                        int num_layers);

  void UpdateQmEncodedFrame(int* encoded_size, int num_updates);

  void UpdateQmRateData(int* target_rate,
                        int* encoder_sent_rate,
                        int* incoming_frame_rate,
                        uint8_t* fraction_lost,
                        int num_updates);

  void UpdateQmContentData(float motion_metric,
                           float spatial_metric,
                           float spatial_metric_horiz,
                           float spatial_metric_vert);

  bool IsSelectedActionCorrect(VCMResolutionScale* qm_scale,
                               float fac_width,
                               float fac_height,
                               float fac_temp,
                               uint16_t new_width,
                               uint16_t new_height,
                               float new_frame_rate);

  void TearDown() {
    delete qm_resolution_;
    delete content_metrics_;
  }
};

TEST_F(QmSelectTest, HandleInputs) {
  
  EXPECT_EQ(-4, qm_resolution_->Initialize(1000, 0, 640, 480, 1));
  EXPECT_EQ(-4, qm_resolution_->Initialize(1000, 30, 640, 0, 1));
  EXPECT_EQ(-4, qm_resolution_->Initialize(1000, 30, 0, 480, 1));

  
  EXPECT_EQ(-7, qm_resolution_->SelectResolution(&qm_scale_));

  VideoContentMetrics* content_metrics = NULL;
  EXPECT_EQ(0, qm_resolution_->Initialize(1000, 30, 640, 480, 1));
  qm_resolution_->UpdateContent(content_metrics);
  
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0, 1.0, 1.0, 640, 480,
                                      30.0f));
}




TEST_F(QmSelectTest, NoActionHighRate) {
  
  
  InitQmNativeData(800, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {800, 800, 800};
  int encoder_sent_rate[] = {800, 800, 800};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  UpdateQmContentData(kTemporalLow, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(0, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      30.0f));
}



TEST_F(QmSelectTest, DownActionLowRate) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(0, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 640, 480,
                                      20.5f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalHigh, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(4, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 640, 480,
                                      20.5f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f, 640, 480,
                                      15.5f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalMedium, kSpatialHigh, kSpatialHigh,
                      kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(7, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f, 640, 480,
                                      15.5f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalHigh, kSpatialMedium, kSpatialMedium,
                      kSpatialMedium);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(5, qm_resolution_->ComputeContentClass());
  
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialMedium, kSpatialMedium,
                      kSpatialMedium);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(2, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f, 640, 480,
                                      15.5f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalMedium, kSpatialMedium, kSpatialMedium,
                      kSpatialMedium);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(8, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 640, 480,
                                      20.5f));
}



TEST_F(QmSelectTest, DownActionHighRateMMOvershoot) {
  
  
  InitQmNativeData(300, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {300, 300, 300};
  int encoder_sent_rate[] = {900, 900, 900};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStressedEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 360, 30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 640, 480,
                                      20.5f));
}



TEST_F(QmSelectTest, NoActionHighRateMMUndershoot) {
  
  
  InitQmNativeData(300, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {300, 300, 300};
  int encoder_sent_rate[] = {100, 100, 100};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kEasyEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      30.0f));
}



TEST_F(QmSelectTest, DownActionBufferUnderflow) {
  
  
  InitQmNativeData(300, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int encoded_size[] = {200, 100, 50, 30, 60, 40, 20, 30, 20, 40};
  UpdateQmEncodedFrame(encoded_size, 10);

  
  int target_rate[] = {300, 300, 300};
  int encoder_sent_rate[] = {450, 450, 450};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStressedEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 360, 30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 640, 480,
                                      20.5f));
}



TEST_F(QmSelectTest, NoActionBufferStable) {
  
  
  InitQmNativeData(350, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int32_t encoded_size[] = {40, 10, 10, 16, 18, 20, 17, 20, 16, 15};
  UpdateQmEncodedFrame(encoded_size, 10);

  
  int target_rate[] = {350, 350, 350};
  int encoder_sent_rate[] = {350, 450, 450};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      30.0f));

  qm_resolution_->ResetDownSamplingState();
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      30.0f));
}


TEST_F(QmSelectTest, LimitDownSpatialAction) {
  
  
  InitQmNativeData(10, 30, 176, 144, 1);

  
  uint16_t codec_width = 176;
  uint16_t codec_height = 144;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(0, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {10, 10, 10};
  int encoder_sent_rate[] = {10, 10, 10};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 176, 144,
                                      30.0f));
}


TEST_F(QmSelectTest, LimitDownTemporalAction) {
  
  
  InitQmNativeData(10, 8, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(8.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {10, 10, 10};
  int encoder_sent_rate[] = {10, 10, 10};
  int incoming_frame_rate[] = {8, 8, 8};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialMedium, kSpatialMedium,
                      kSpatialMedium);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(2, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      8.0f));
}



TEST_F(QmSelectTest, 2StageDownSpatialUpSpatial) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                    fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  
  qm_resolution_->ResetRates();
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  
  int target_rate2[] = {400, 400, 400, 400, 400};
  int encoder_sent_rate2[] = {400, 400, 400, 400, 400};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  float scale = (4.0f / 3.0f) / 2.0f;
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, scale, scale, 1.0f, 480, 360,
                                      30.0f));

  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f, 1.0f,
                                      640, 480, 30.0f));
}



TEST_F(QmSelectTest, 2StageDownSpatialUpSpatialUndershoot) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                    fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  
  
  qm_resolution_->ResetRates();
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  
  int target_rate2[] = {200, 200, 200, 200, 200};
  int encoder_sent_rate2[] = {50, 50, 50, 50, 50};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kEasyEncoding, qm_resolution_->GetEncoderState());
  float scale = (4.0f / 3.0f) / 2.0f;
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, scale, scale, 1.0f, 480, 360,
                                      30.0f));

  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f, 1.0f,
                                      640, 480, 30.0f));
}



TEST_F(QmSelectTest, 2StageDownSpatialNoActionUp) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                    fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  
  qm_resolution_->ResetRates();
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  
  int target_rate2[] = {400, 400, 400, 400, 400};
  int encoder_sent_rate2[] = {1000, 1000, 1000, 1000, 1000};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kStressedEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 320, 240,
                                      30.0f));
}



TEST_F(QmSelectTest, 2StatgeDownTemporalUpTemporal) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f, 640, 480,
                                      15.5f));

  
  qm_resolution_->ResetRates();
  
  int target_rate2[] = {400, 400, 400, 400, 400};
  int encoder_sent_rate2[] = {400, 400, 400, 400, 400};
  int incoming_frame_rate2[] = {15, 15, 15, 15, 15};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 0.5f, 640, 480,
                                      30.0f));
}



TEST_F(QmSelectTest, 2StatgeDownTemporalUpTemporalUndershoot) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                    fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f, 640, 480,
                                      15.5f));

  
  qm_resolution_->ResetRates();
  
  int target_rate2[] = {150, 150, 150, 150, 150};
  int encoder_sent_rate2[] = {50, 50, 50, 50, 50};
  int incoming_frame_rate2[] = {15, 15, 15, 15, 15};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kEasyEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 0.5f, 640, 480,
                                      30.0f));
}



TEST_F(QmSelectTest, 2StageDownTemporalNoActionUp) {
  
  
  InitQmNativeData(50, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {50, 50, 50};
  int encoder_sent_rate[] = {50, 50, 50};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1, 1, 2, 640, 480, 15.5f));

  
  qm_resolution_->UpdateCodecParameters(15.0f, codec_width, codec_height);
  qm_resolution_->ResetRates();
  
  int target_rate2[] = {600, 600, 600, 600, 600};
  int encoder_sent_rate2[] = {1000, 1000, 1000, 1000, 1000};
  int incoming_frame_rate2[] = {15, 15, 15, 15, 15};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kStressedEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 640, 480,
                                      15.0f));
}


TEST_F(QmSelectTest, 3StageDownSpatialTemporlaUpSpatialTemporal) {
  
  
  InitQmNativeData(80, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {80, 80, 80};
  int encoder_sent_rate[] = {80, 80, 80};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));

  
  qm_resolution_->ResetRates();
  int target_rate2[] = {40, 40, 40, 40, 40};
  int encoder_sent_rate2[] = {40, 40, 40, 40, 40};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                    fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 320, 240,
                                      20.5f));

  
  
  qm_resolution_->ResetRates();
  
  int target_rate3[] = {1000, 1000, 1000, 1000, 1000};
  int encoder_sent_rate3[] = {1000, 1000, 1000, 1000, 1000};
  int incoming_frame_rate3[] = {20, 20, 20, 20, 20};
  uint8_t fraction_lost3[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate3, encoder_sent_rate3, incoming_frame_rate3,
                   fraction_lost3, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  float scale = (4.0f / 3.0f) / 2.0f;
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, scale, scale, 2.0f / 3.0f,
                                      480, 360, 30.0f));

  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f, 1.0f,
                                      640, 480, 30.0f));
}


TEST_F(QmSelectTest, NoActionTooMuchDownSampling) {
  
  
  InitQmNativeData(150, 30, 1280, 720, 1);

  
  uint16_t codec_width = 1280;
  uint16_t codec_height = 720;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(7, qm_resolution_->GetImageType(codec_width, codec_height));

  
  int target_rate[] = {150, 150, 150};
  int encoder_sent_rate[] = {150, 150, 150};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 640, 360,
              30.0f));

  
  
  qm_resolution_->ResetRates();
  qm_resolution_->UpdateCodecParameters(10.0f, 640, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(640, 360));
  
  int target_rate2[] = {70, 70, 70, 70, 70};
  int encoder_sent_rate2[] = {70, 70, 70, 70, 70};
  int incoming_frame_rate2[] = {10, 10, 10, 10, 10};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialMedium, kSpatialMedium,
                      kSpatialMedium);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(5, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 270, 10.0f));

  
  
  qm_resolution_->ResetRates();
  qm_resolution_->UpdateCodecParameters(10.0f, 480, 270);
  EXPECT_EQ(3, qm_resolution_->GetImageType(480, 270));
  
  int target_rate3[] = {10, 10, 10, 10, 10};
  int encoder_sent_rate3[] = {10, 10, 10, 10, 10};
  int incoming_frame_rate3[] = {10, 10, 10, 10, 10};
  uint8_t fraction_lost3[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate3, encoder_sent_rate3, incoming_frame_rate3,
                   fraction_lost3, 5);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(5, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.0f, 480, 270,
                                      10.0f));
}





TEST_F(QmSelectTest, MultipleStagesCheckActionHistory1) {
  
  
  InitQmNativeData(150, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int target_rate[] = {150, 150, 150};
  int encoder_sent_rate[] = {150, 150, 150};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 360, 30.0f));
  
  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  qm_resolution_->ResetRates();
  int target_rate2[] = {100, 100, 100, 100, 100};
  int encoder_sent_rate2[] = {100, 100, 100, 100, 100};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 480, 360,
                                      20.5f));

  
  qm_resolution_->UpdateCodecParameters(20.0f, 480, 360);
  qm_resolution_->ResetRates();
  int target_rate3[] = {80, 80, 80, 80, 80};
  int encoder_sent_rate3[] = {80, 80, 80, 80, 80};
  int incoming_frame_rate3[] = {20, 20, 20, 20, 20};
  uint8_t fraction_lost3[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate3, encoder_sent_rate3, incoming_frame_rate3,
                    fraction_lost3, 5);

  
  
  UpdateQmContentData(kTemporalHigh, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  
  
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      20.0f));

  
  

  
  qm_resolution_->UpdateCodecParameters(15.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  qm_resolution_->ResetRates();
  
  int target_rate4[] = {1000, 1000, 1000, 1000, 1000};
  int encoder_sent_rate4[] = {1000, 1000, 1000, 1000, 1000};
  int incoming_frame_rate4[] = {15, 15, 15, 15, 15};
  uint8_t fraction_lost4[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate4, encoder_sent_rate4, incoming_frame_rate4,
                   fraction_lost4, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(3, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  float scale = (4.0f / 3.0f) / 2.0f;
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, scale, scale, 2.0f / 3.0f, 480,
                                      360, 30.0f));

  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f, 1.0f,
                                      640, 480, 30.0f));
}




TEST_F(QmSelectTest, MultipleStagesCheckActionHistory2) {
  
  
  InitQmNativeData(80, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int target_rate[] = {80, 80, 80};
  int encoder_sent_rate[] = {80, 80, 80};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  qm_resolution_->ResetRates();
  int target_rate2[] = {40, 40, 40, 40, 40};
  int encoder_sent_rate2[] = {40, 40, 40, 40, 40};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialHigh, kSpatialHigh,
                      kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(7, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 320, 240,
                                      20.5f));

  
  qm_resolution_->UpdateCodecParameters(20.0f, 320, 240);
  qm_resolution_->ResetRates();
  
  int target_rate3[] = {150, 150, 150, 150, 150};
  int encoder_sent_rate3[] = {150, 150, 150, 150, 150};
  int incoming_frame_rate3[] = {20, 20, 20, 20, 20};
  uint8_t fraction_lost3[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate3, encoder_sent_rate3, incoming_frame_rate3,
                   fraction_lost3, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(7, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f / 3.0f, 320,
                                      240, 30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 320, 240);
  EXPECT_EQ(2, qm_resolution_->GetImageType(320, 240));
  qm_resolution_->ResetRates();
  int target_rate4[] = {40, 40, 40, 40, 40};
  int encoder_sent_rate4[] = {40, 40, 40, 40, 40};
  int incoming_frame_rate4[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost4[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate4, encoder_sent_rate4, incoming_frame_rate4,
                   fraction_lost4, 5);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 320, 240,
                                      20.5f));

  
  qm_resolution_->UpdateCodecParameters(20.5f, 320, 240);
  qm_resolution_->ResetRates();
  
  int target_rate5[] = {1000, 1000, 1000, 1000, 1000};
  int encoder_sent_rate5[] = {1000, 1000, 1000, 1000, 1000};
  int incoming_frame_rate5[] = {20, 20, 20, 20, 20};
  uint8_t fraction_lost5[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate5, encoder_sent_rate5, incoming_frame_rate5,
                   fraction_lost5, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  float scale = (4.0f / 3.0f) / 2.0f;
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, scale, scale, 2.0f / 3.0f,
                                      480, 360, 30.0f));

  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f, 1.0f,
                                      640, 480, 30.0f));
}




TEST_F(QmSelectTest, MultipleStagesCheckActionHistory3) {
  
  
  InitQmNativeData(100, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int target_rate[] = {100, 100, 100};
  int encoder_sent_rate[] = {100, 100, 100};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 360, 30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  qm_resolution_->ResetRates();
  int target_rate2[] = {100, 100, 100, 100, 100};
  int encoder_sent_rate2[] = {100, 100, 100, 100, 100};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalLow, kSpatialHigh, kSpatialHigh, kSpatialHigh);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 1.5f, 480, 360,
                                      20.5f));

  
  qm_resolution_->UpdateCodecParameters(20.5f, 480, 360);
  qm_resolution_->ResetRates();
  
  int target_rate3[] = {250, 250, 250, 250, 250};
  int encoder_sent_rate3[] = {250, 250, 250, 250, 250};
  int incoming_frame_rate3[] = {20, 20, 20, 20, 120};
  uint8_t fraction_lost3[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate3, encoder_sent_rate3, incoming_frame_rate3,
                   fraction_lost3, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(1, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 1.0f, 1.0f, 2.0f / 3.0f, 480,
                                      360, 30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  qm_resolution_->ResetRates();
  int target_rate4[] = {500, 500, 500, 500, 500};
  int encoder_sent_rate4[] = {500, 500, 500, 500, 500};
  int incoming_frame_rate4[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost4[] = {30, 30, 30, 30, 30};
  UpdateQmRateData(target_rate4, encoder_sent_rate4, incoming_frame_rate4,
                   fraction_lost4, 5);

  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 3.0f / 4.0f, 3.0f / 4.0f,
                                      1.0f, 640, 480, 30.0f));
}


TEST_F(QmSelectTest, ConvertThreeQuartersToOneHalf) {
  
  
  InitQmNativeData(150, 30, 640, 480, 1);

  
  uint16_t codec_width = 640;
  uint16_t codec_height = 480;
  qm_resolution_->UpdateCodecParameters(30.0f, codec_width, codec_height);
  EXPECT_EQ(5, qm_resolution_->GetImageType(codec_width, codec_height));

  
  
  int target_rate[] = {150, 150, 150};
  int encoder_sent_rate[] = {150, 150, 150};
  int incoming_frame_rate[] = {30, 30, 30};
  uint8_t fraction_lost[] = {10, 10, 10};
  UpdateQmRateData(target_rate, encoder_sent_rate, incoming_frame_rate,
                   fraction_lost, 3);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 4.0f / 3.0f, 4.0f / 3.0f,
                                      1.0f, 480, 360, 30.0f));

  
  qm_resolution_->UpdateCodecParameters(30.0f, 480, 360);
  EXPECT_EQ(4, qm_resolution_->GetImageType(480, 360));
  qm_resolution_->ResetRates();
  int target_rate2[] = {100, 100, 100, 100, 100};
  int encoder_sent_rate2[] = {100, 100, 100, 100, 100};
  int incoming_frame_rate2[] = {30, 30, 30, 30, 30};
  uint8_t fraction_lost2[] = {10, 10, 10, 10, 10};
  UpdateQmRateData(target_rate2, encoder_sent_rate2, incoming_frame_rate2,
                   fraction_lost2, 5);

  
  
  UpdateQmContentData(kTemporalMedium, kSpatialLow, kSpatialLow, kSpatialLow);
  EXPECT_EQ(0, qm_resolution_->SelectResolution(&qm_scale_));
  EXPECT_EQ(6, qm_resolution_->ComputeContentClass());
  EXPECT_EQ(kStableEncoding, qm_resolution_->GetEncoderState());
  EXPECT_TRUE(IsSelectedActionCorrect(qm_scale_, 2.0f, 2.0f, 1.0f, 320, 240,
                                      30.0f));
}

void QmSelectTest::InitQmNativeData(float initial_bit_rate,
                                    int user_frame_rate,
                                    int native_width,
                                    int native_height,
                                    int num_layers) {
  EXPECT_EQ(0, qm_resolution_->Initialize(initial_bit_rate,
                                          user_frame_rate,
                                          native_width,
                                          native_height,
                                          num_layers));
}

void QmSelectTest::UpdateQmContentData(float motion_metric,
                                       float spatial_metric,
                                       float spatial_metric_horiz,
                                       float spatial_metric_vert) {
  content_metrics_->motion_magnitude = motion_metric;
  content_metrics_->spatial_pred_err = spatial_metric;
  content_metrics_->spatial_pred_err_h = spatial_metric_horiz;
  content_metrics_->spatial_pred_err_v = spatial_metric_vert;
  qm_resolution_->UpdateContent(content_metrics_);
}

void QmSelectTest::UpdateQmEncodedFrame(int* encoded_size, int num_updates) {
  FrameType frame_type = kVideoFrameDelta;
  for (int i = 0; i < num_updates; ++i) {
    
    int32_t encoded_size_update = 1000 * encoded_size[i] / 8;
    qm_resolution_->UpdateEncodedSize(encoded_size_update, frame_type);
  }
}

void QmSelectTest::UpdateQmRateData(int* target_rate,
                                    int* encoder_sent_rate,
                                    int* incoming_frame_rate,
                                    uint8_t* fraction_lost,
                                    int num_updates) {
  for (int i = 0; i < num_updates; ++i) {
    float target_rate_update = target_rate[i];
    float encoder_sent_rate_update = encoder_sent_rate[i];
    float incoming_frame_rate_update = incoming_frame_rate[i];
    uint8_t fraction_lost_update = fraction_lost[i];
    qm_resolution_->UpdateRates(target_rate_update,
                                encoder_sent_rate_update,
                                incoming_frame_rate_update,
                                fraction_lost_update);
  }
}



bool QmSelectTest::IsSelectedActionCorrect(VCMResolutionScale* qm_scale,
                                           float fac_width,
                                           float fac_height,
                                           float fac_temp,
                                           uint16_t new_width,
                                           uint16_t new_height,
                                           float new_frame_rate) {
  if (qm_scale->spatial_width_fact == fac_width &&
      qm_scale->spatial_height_fact == fac_height &&
      qm_scale->temporal_fact == fac_temp &&
      qm_scale->codec_width == new_width &&
      qm_scale->codec_height == new_height &&
      qm_scale->frame_rate == new_frame_rate) {
    return true;
  } else {
    return false;
  }
}
}  
