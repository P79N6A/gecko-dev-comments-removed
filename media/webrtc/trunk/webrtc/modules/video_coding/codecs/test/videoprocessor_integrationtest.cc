









#include <math.h>

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/modules/video_coding/codecs/test/packet_manipulator.h"
#include "webrtc/modules/video_coding/codecs/test/videoprocessor.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/frame_reader.h"
#include "webrtc/test/testsupport/frame_writer.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/test/testsupport/metrics/video_metrics.h"
#include "webrtc/test/testsupport/packet_reader.h"
#include "webrtc/typedefs.h"

namespace webrtc {



const int kMaxNumRateUpdates = 3;

const int kPercTargetvsActualMismatch = 20;
const int kBaseKeyFrameInterval = 3000;


struct CodecConfigPars {
  float packet_loss;
  int num_temporal_layers;
  int key_frame_interval;
  bool error_concealment_on;
  bool denoising_on;
  bool frame_dropper_on;
  bool spatial_resize_on;
};


struct QualityMetrics {
  double minimum_avg_psnr;
  double minimum_min_psnr;
  double minimum_avg_ssim;
  double minimum_min_ssim;
};




struct RateProfile {
  int target_bit_rate[kMaxNumRateUpdates];
  int input_frame_rate[kMaxNumRateUpdates];
  int frame_index_rate_update[kMaxNumRateUpdates + 1];
  int num_frames;
};






struct RateControlMetrics {
  int max_num_dropped_frames;
  int max_key_frame_size_mismatch;
  int max_delta_frame_size_mismatch;
  int max_encoding_rate_mismatch;
  int max_time_hit_target;
  int num_spatial_resizes;
};



const int kCIFWidth = 352;
const int kCIFHeight = 288;
const int kNbrFramesShort = 100;  
const int kNbrFramesLong = 299;


const float kInitialBufferSize = 0.5f;
const float kOptimalBufferSize = 0.6f;
const float kScaleKeyFrameSize = 0.5f;









class VideoProcessorIntegrationTest: public testing::Test {
 protected:
  VideoEncoder* encoder_;
  VideoDecoder* decoder_;
  webrtc::test::FrameReader* frame_reader_;
  webrtc::test::FrameWriter* frame_writer_;
  webrtc::test::PacketReader packet_reader_;
  webrtc::test::PacketManipulator* packet_manipulator_;
  webrtc::test::Stats stats_;
  webrtc::test::TestConfig config_;
  VideoCodec codec_settings_;
  webrtc::test::VideoProcessor* processor_;

  
  
  int num_frames_per_update_[3];
  float sum_frame_size_mismatch_[3];
  float sum_encoded_frame_size_[3];
  float encoding_bitrate_[3];
  float per_frame_bandwidth_[3];
  float bit_rate_layer_[3];
  float frame_rate_layer_[3];
  int num_frames_total_;
  float sum_encoded_frame_size_total_;
  float encoding_bitrate_total_;
  float perc_encoding_rate_mismatch_;
  int num_frames_to_hit_target_;
  bool encoding_rate_within_target_;
  int bit_rate_;
  int frame_rate_;
  int layer_;
  float target_size_key_frame_initial_;
  float target_size_key_frame_;
  float sum_key_frame_size_mismatch_;
  int num_key_frames_;
  float start_bitrate_;

  
  float packet_loss_;
  int num_temporal_layers_;
  int key_frame_interval_;
  bool error_concealment_on_;
  bool denoising_on_;
  bool frame_dropper_on_;
  bool spatial_resize_on_;


  VideoProcessorIntegrationTest() {}
  virtual ~VideoProcessorIntegrationTest() {}

  void SetUpCodecConfig() {
    encoder_ = VP8Encoder::Create();
    decoder_ = VP8Decoder::Create();

    
    
    config_.input_filename =
        webrtc::test::ResourcePath("foreman_cif", "yuv");
    config_.output_filename = tmpnam(NULL);
    config_.frame_length_in_bytes = CalcBufferSize(kI420,
                                                   kCIFWidth, kCIFHeight);
    config_.verbose = false;
    
    config_.use_single_core = true;
    
    config_.keyframe_interval = key_frame_interval_;
    config_.networking_config.packet_loss_probability = packet_loss_;

    
    VideoCodingModule::Codec(kVideoCodecVP8, &codec_settings_);
    config_.codec_settings = &codec_settings_;
    config_.codec_settings->startBitrate = start_bitrate_;
    config_.codec_settings->width = kCIFWidth;
    config_.codec_settings->height = kCIFHeight;
    
    config_.codec_settings->codecSpecific.VP8.errorConcealmentOn =
        error_concealment_on_;
    config_.codec_settings->codecSpecific.VP8.denoisingOn =
        denoising_on_;
    config_.codec_settings->codecSpecific.VP8.numberOfTemporalLayers =
        num_temporal_layers_;
    config_.codec_settings->codecSpecific.VP8.frameDroppingOn =
        frame_dropper_on_;
    config_.codec_settings->codecSpecific.VP8.automaticResizeOn =
        spatial_resize_on_;
    config_.codec_settings->codecSpecific.VP8.keyFrameInterval =
        kBaseKeyFrameInterval;

    frame_reader_ =
        new webrtc::test::FrameReaderImpl(config_.input_filename,
                                          config_.frame_length_in_bytes);
    frame_writer_ =
        new webrtc::test::FrameWriterImpl(config_.output_filename,
                                          config_.frame_length_in_bytes);
    ASSERT_TRUE(frame_reader_->Init());
    ASSERT_TRUE(frame_writer_->Init());

    packet_manipulator_ = new webrtc::test::PacketManipulatorImpl(
        &packet_reader_, config_.networking_config, config_.verbose);
    processor_ = new webrtc::test::VideoProcessorImpl(encoder_, decoder_,
                                                      frame_reader_,
                                                      frame_writer_,
                                                      packet_manipulator_,
                                                      config_, &stats_);
    ASSERT_TRUE(processor_->Init());
  }

  
  
  void ResetRateControlMetrics(int num_frames) {
    for (int i = 0; i < num_temporal_layers_; i++) {
      num_frames_per_update_[i] = 0;
      sum_frame_size_mismatch_[i] = 0.0f;
      sum_encoded_frame_size_[i] = 0.0f;
      encoding_bitrate_[i] = 0.0f;
      
      per_frame_bandwidth_[i] = static_cast<float>(bit_rate_layer_[i]) /
             static_cast<float>(frame_rate_layer_[i]);
    }
    
    float max_key_size = kScaleKeyFrameSize * kOptimalBufferSize * frame_rate_;
    
    
    
    
    
    target_size_key_frame_ = 0.5 * (3 + max_key_size) * per_frame_bandwidth_[0];
    num_frames_total_ = 0;
    sum_encoded_frame_size_total_ = 0.0f;
    encoding_bitrate_total_ = 0.0f;
    perc_encoding_rate_mismatch_ = 0.0f;
    num_frames_to_hit_target_ = num_frames;
    encoding_rate_within_target_ = false;
    sum_key_frame_size_mismatch_ = 0.0;
    num_key_frames_ = 0;
  }

  
  void UpdateRateControlMetrics(int frame_num, VideoFrameType frame_type) {
    int encoded_frame_size = processor_->EncodedFrameSize();
    float encoded_size_kbits = encoded_frame_size * 8.0f / 1000.0f;
    
    
    if (frame_type == kDeltaFrame) {
      
      sum_frame_size_mismatch_[layer_] += fabs(encoded_size_kbits -
                                               per_frame_bandwidth_[layer_]) /
                                               per_frame_bandwidth_[layer_];
    } else {
      float target_size = (frame_num == 1) ? target_size_key_frame_initial_ :
          target_size_key_frame_;
      sum_key_frame_size_mismatch_ += fabs(encoded_size_kbits - target_size) /
          target_size;
      num_key_frames_ += 1;
    }
    sum_encoded_frame_size_[layer_] += encoded_size_kbits;
    
    
    encoding_bitrate_[layer_] = sum_encoded_frame_size_[layer_] *
        frame_rate_layer_[layer_] /
        num_frames_per_update_[layer_];
    
    sum_encoded_frame_size_total_ += encoded_size_kbits;
    encoding_bitrate_total_ = sum_encoded_frame_size_total_ * frame_rate_ /
        num_frames_total_;
    perc_encoding_rate_mismatch_ =  100 * fabs(encoding_bitrate_total_ -
                                               bit_rate_) / bit_rate_;
    if (perc_encoding_rate_mismatch_ < kPercTargetvsActualMismatch &&
        !encoding_rate_within_target_) {
      num_frames_to_hit_target_ = num_frames_total_;
      encoding_rate_within_target_ = true;
    }
  }

  
  void VerifyRateControl(int update_index,
                         int max_key_frame_size_mismatch,
                         int max_delta_frame_size_mismatch,
                         int max_encoding_rate_mismatch,
                         int max_time_hit_target,
                         int max_num_dropped_frames,
                         int num_spatial_resizes) {
    int num_dropped_frames = processor_->NumberDroppedFrames();
    int num_resize_actions = processor_->NumberSpatialResizes();
    printf("For update #: %d,\n "
        " Target Bitrate: %d,\n"
        " Encoding bitrate: %f,\n"
        " Frame rate: %d \n",
        update_index, bit_rate_, encoding_bitrate_total_, frame_rate_);
    printf(" Number of frames to approach target rate = %d, \n"
           " Number of dropped frames = %d, \n"
           " Number of spatial resizes = %d, \n",
           num_frames_to_hit_target_, num_dropped_frames, num_resize_actions);
    EXPECT_LE(perc_encoding_rate_mismatch_, max_encoding_rate_mismatch);
    if (num_key_frames_ > 0) {
      int perc_key_frame_size_mismatch = 100 * sum_key_frame_size_mismatch_ /
              num_key_frames_;
      printf(" Number of Key frames: %d \n"
             " Key frame rate mismatch: %d \n",
             num_key_frames_, perc_key_frame_size_mismatch);
      EXPECT_LE(perc_key_frame_size_mismatch, max_key_frame_size_mismatch);
    }
    printf("\n");
    printf("Rates statistics for Layer data \n");
    for (int i = 0; i < num_temporal_layers_ ; i++) {
      printf("Layer #%d \n", i);
      int perc_frame_size_mismatch = 100 * sum_frame_size_mismatch_[i] /
        num_frames_per_update_[i];
      int perc_encoding_rate_mismatch = 100 * fabs(encoding_bitrate_[i] -
                                                   bit_rate_layer_[i]) /
                                                   bit_rate_layer_[i];
      printf(" Target Layer Bit rate: %f \n"
          " Layer frame rate: %f, \n"
          " Layer per frame bandwidth: %f, \n"
          " Layer Encoding bit rate: %f, \n"
          " Layer Percent frame size mismatch: %d,  \n"
          " Layer Percent encoding rate mismatch = %d, \n"
          " Number of frame processed per layer = %d \n",
          bit_rate_layer_[i], frame_rate_layer_[i], per_frame_bandwidth_[i],
          encoding_bitrate_[i], perc_frame_size_mismatch,
          perc_encoding_rate_mismatch, num_frames_per_update_[i]);
      EXPECT_LE(perc_frame_size_mismatch, max_delta_frame_size_mismatch);
      EXPECT_LE(perc_encoding_rate_mismatch, max_encoding_rate_mismatch);
    }
    printf("\n");
    EXPECT_LE(num_frames_to_hit_target_, max_time_hit_target);
    EXPECT_LE(num_dropped_frames, max_num_dropped_frames);
    EXPECT_EQ(num_resize_actions, num_spatial_resizes);
  }

  
  void LayerIndexForFrame(int frame_number) {
    if (num_temporal_layers_ == 1) {
      layer_ = 0;
    } else if (num_temporal_layers_ == 2) {
        
        
        if (frame_number % 2 == 0) {
          layer_ = 0;
        } else {
          layer_ = 1;
        }
    } else if (num_temporal_layers_ == 3) {
      
      
      
      if (frame_number % 4 == 0) {
        layer_ = 0;
      } else if ((frame_number + 2) % 4 == 0) {
        layer_ = 1;
      } else if ((frame_number + 1) % 2 == 0) {
        layer_ = 2;
      }
    } else {
      assert(false);  
    }
  }

  
  void SetLayerRates() {
    assert(num_temporal_layers_<= 3);
    for (int i = 0; i < num_temporal_layers_; i++) {
      float bit_rate_ratio =
          kVp8LayerRateAlloction[num_temporal_layers_ - 1][i];
      if (i > 0) {
        float bit_rate_delta_ratio = kVp8LayerRateAlloction
            [num_temporal_layers_ - 1][i] -
            kVp8LayerRateAlloction[num_temporal_layers_ - 1][i - 1];
        bit_rate_layer_[i] = bit_rate_ * bit_rate_delta_ratio;
      } else {
        bit_rate_layer_[i] = bit_rate_ * bit_rate_ratio;
      }
      frame_rate_layer_[i] = frame_rate_ / static_cast<float>(
          1 << (num_temporal_layers_ - 1));
    }
    if (num_temporal_layers_ == 3) {
      frame_rate_layer_[2] = frame_rate_ / 2.0f;
    }
  }

  VideoFrameType FrameType(int frame_number) {
    if (frame_number == 0 || ((frame_number) % key_frame_interval_ == 0 &&
        key_frame_interval_ > 0)) {
      return kKeyFrame;
    } else {
      return kDeltaFrame;
    }
  }

  void TearDown() {
    delete processor_;
    delete packet_manipulator_;
    delete frame_writer_;
    delete frame_reader_;
    delete decoder_;
    delete encoder_;
  }

  
  void ProcessFramesAndVerify(QualityMetrics quality_metrics,
                              RateProfile rate_profile,
                              CodecConfigPars process,
                              RateControlMetrics* rc_metrics) {
    
    start_bitrate_ = rate_profile.target_bit_rate[0];
    packet_loss_ = process.packet_loss;
    key_frame_interval_ = process.key_frame_interval;
    num_temporal_layers_ = process.num_temporal_layers;
    error_concealment_on_ = process.error_concealment_on;
    denoising_on_ = process.denoising_on;
    frame_dropper_on_ = process.frame_dropper_on;
    spatial_resize_on_ = process.spatial_resize_on;
    SetUpCodecConfig();
    
    bit_rate_ =  rate_profile.target_bit_rate[0];
    frame_rate_ = rate_profile.input_frame_rate[0];
    SetLayerRates();
    
    target_size_key_frame_initial_ = 0.5 * kInitialBufferSize *
        bit_rate_layer_[0];
    processor_->SetRates(bit_rate_, frame_rate_);
    
    int num_frames = rate_profile.num_frames;
    int update_index = 0;
    ResetRateControlMetrics(
        rate_profile.frame_index_rate_update[update_index + 1]);
    int frame_number = 0;
    VideoFrameType frame_type = kDeltaFrame;
    while (processor_->ProcessFrame(frame_number) &&
        frame_number < num_frames) {
      
      LayerIndexForFrame(frame_number);
      frame_type = FrameType(frame_number);
      
      ++frame_number;
      
      ++num_frames_per_update_[layer_];
      ++num_frames_total_;
      UpdateRateControlMetrics(frame_number, frame_type);
      
      
      if (frame_number ==
          rate_profile.frame_index_rate_update[update_index + 1]) {
        VerifyRateControl(
            update_index,
            rc_metrics[update_index].max_key_frame_size_mismatch,
            rc_metrics[update_index].max_delta_frame_size_mismatch,
            rc_metrics[update_index].max_encoding_rate_mismatch,
            rc_metrics[update_index].max_time_hit_target,
            rc_metrics[update_index].max_num_dropped_frames,
            rc_metrics[update_index].num_spatial_resizes);
        
        ++update_index;
        bit_rate_ =  rate_profile.target_bit_rate[update_index];
        frame_rate_ = rate_profile.input_frame_rate[update_index];
        SetLayerRates();
        ResetRateControlMetrics(rate_profile.
                                frame_index_rate_update[update_index + 1]);
        processor_->SetRates(bit_rate_, frame_rate_);
      }
    }
    VerifyRateControl(
        update_index,
        rc_metrics[update_index].max_key_frame_size_mismatch,
        rc_metrics[update_index].max_delta_frame_size_mismatch,
        rc_metrics[update_index].max_encoding_rate_mismatch,
        rc_metrics[update_index].max_time_hit_target,
        rc_metrics[update_index].max_num_dropped_frames,
        rc_metrics[update_index].num_spatial_resizes);
    EXPECT_EQ(num_frames, frame_number);
    EXPECT_EQ(num_frames + 1, static_cast<int>(stats_.stats_.size()));

    
    EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
    EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, decoder_->Release());
    
    frame_reader_->Close();
    frame_writer_->Close();

    
    webrtc::test::QualityMetricsResult psnr_result, ssim_result;
    EXPECT_EQ(0, webrtc::test::I420MetricsFromFiles(
        config_.input_filename.c_str(),
        config_.output_filename.c_str(),
        config_.codec_settings->width,
        config_.codec_settings->height,
        &psnr_result,
        &ssim_result));
    printf("PSNR avg: %f, min: %f    SSIM avg: %f, min: %f\n",
           psnr_result.average, psnr_result.min,
           ssim_result.average, ssim_result.min);
    stats_.PrintSummary();
    EXPECT_GT(psnr_result.average, quality_metrics.minimum_avg_psnr);
    EXPECT_GT(psnr_result.min, quality_metrics.minimum_min_psnr);
    EXPECT_GT(ssim_result.average, quality_metrics.minimum_avg_ssim);
    EXPECT_GT(ssim_result.min, quality_metrics.minimum_min_ssim);
    if (!remove(config_.output_filename.c_str())) {
      fprintf(stderr, "Failed to remove temporary file!");
    }
  }
};

void SetRateProfilePars(RateProfile* rate_profile,
                        int update_index,
                        int bit_rate,
                        int frame_rate,
                        int frame_index_rate_update) {
  rate_profile->target_bit_rate[update_index] = bit_rate;
  rate_profile->input_frame_rate[update_index] = frame_rate;
  rate_profile->frame_index_rate_update[update_index] = frame_index_rate_update;
}

void SetCodecParameters(CodecConfigPars* process_settings,
                        float packet_loss,
                        int key_frame_interval,
                        int num_temporal_layers,
                        bool error_concealment_on,
                        bool denoising_on,
                        bool frame_dropper_on,
                        bool spatial_resize_on) {
  process_settings->packet_loss = packet_loss;
  process_settings->key_frame_interval =  key_frame_interval;
  process_settings->num_temporal_layers = num_temporal_layers,
  process_settings->error_concealment_on = error_concealment_on;
  process_settings->denoising_on = denoising_on;
  process_settings->frame_dropper_on = frame_dropper_on;
  process_settings->spatial_resize_on = spatial_resize_on;
}

void SetQualityMetrics(QualityMetrics* quality_metrics,
                       double minimum_avg_psnr,
                       double minimum_min_psnr,
                       double minimum_avg_ssim,
                       double minimum_min_ssim) {
  quality_metrics->minimum_avg_psnr = minimum_avg_psnr;
  quality_metrics->minimum_min_psnr = minimum_min_psnr;
  quality_metrics->minimum_avg_ssim = minimum_avg_ssim;
  quality_metrics->minimum_min_ssim = minimum_min_ssim;
}

void SetRateControlMetrics(RateControlMetrics* rc_metrics,
                           int update_index,
                           int max_num_dropped_frames,
                           int max_key_frame_size_mismatch,
                           int max_delta_frame_size_mismatch,
                           int max_encoding_rate_mismatch,
                           int max_time_hit_target,
                           int num_spatial_resizes) {
  rc_metrics[update_index].max_num_dropped_frames = max_num_dropped_frames;
  rc_metrics[update_index].max_key_frame_size_mismatch =
      max_key_frame_size_mismatch;
  rc_metrics[update_index].max_delta_frame_size_mismatch =
      max_delta_frame_size_mismatch;
  rc_metrics[update_index].max_encoding_rate_mismatch =
      max_encoding_rate_mismatch;
  rc_metrics[update_index].max_time_hit_target = max_time_hit_target;
  rc_metrics[update_index].num_spatial_resizes = num_spatial_resizes;
}




TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(ProcessZeroPacketLoss)) {
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 500, 30, 0);
  rate_profile.frame_index_rate_update[1] = kNbrFramesShort + 1;
  rate_profile.num_frames = kNbrFramesShort;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.0f, -1, 1, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 36.95, 33.0, 0.90, 0.90);
  
  RateControlMetrics rc_metrics[1];
  SetRateControlMetrics(rc_metrics, 0, 0, 40, 20, 10, 15, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}



TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(Process5PercentPacketLoss)) {
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 500, 30, 0);
  rate_profile.frame_index_rate_update[1] = kNbrFramesShort + 1;
  rate_profile.num_frames = kNbrFramesShort;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.05f, -1, 1, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 20.0, 16.0, 0.60, 0.40);
  
  RateControlMetrics rc_metrics[1];
  SetRateControlMetrics(rc_metrics, 0, 0, 40, 20, 10, 15, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}



TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(Process10PercentPacketLoss)) {
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 500, 30, 0);
  rate_profile.frame_index_rate_update[1] = kNbrFramesShort + 1;
  rate_profile.num_frames = kNbrFramesShort;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.1f, -1, 1, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 19.0, 16.0, 0.50, 0.35);
  
  RateControlMetrics rc_metrics[1];
  SetRateControlMetrics(rc_metrics, 0, 0, 40, 20, 10, 15, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}





TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(ProcessNoLossChangeBitRate)) {
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 200, 30, 0);
  SetRateProfilePars(&rate_profile, 1, 800, 30, 100);
  SetRateProfilePars(&rate_profile, 2, 500, 30, 200);
  rate_profile.frame_index_rate_update[3] = kNbrFramesLong + 1;
  rate_profile.num_frames = kNbrFramesLong;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.0f, -1, 1, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 34.0, 32.0, 0.85, 0.80);
  
  RateControlMetrics rc_metrics[3];
  SetRateControlMetrics(rc_metrics, 0, 0, 45, 20, 10, 15, 0);
  SetRateControlMetrics(rc_metrics, 1, 0, 0, 25, 20, 10, 0);
  SetRateControlMetrics(rc_metrics, 2, 0, 0, 25, 15, 10, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}








TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(ProcessNoLossChangeFrameRateFrameDrop)) {
  config_.networking_config.packet_loss_probability = 0;
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 80, 24, 0);
  SetRateProfilePars(&rate_profile, 1, 80, 15, 100);
  SetRateProfilePars(&rate_profile, 2, 80, 10, 200);
  rate_profile.frame_index_rate_update[3] = kNbrFramesLong + 1;
  rate_profile.num_frames = kNbrFramesLong;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.0f, -1, 1, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 31.0, 22.0, 0.80, 0.65);
  
  RateControlMetrics rc_metrics[3];
  SetRateControlMetrics(rc_metrics, 0, 40, 20, 75, 15, 60, 0);
  SetRateControlMetrics(rc_metrics, 1, 10, 0, 25, 10, 35, 0);
  SetRateControlMetrics(rc_metrics, 2, 0, 0, 20, 10, 15, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}







TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(ProcessNoLossSpatialResizeFrameDrop)) {
  config_.networking_config.packet_loss_probability = 0;
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 100, 30, 0);
  SetRateProfilePars(&rate_profile, 1, 200, 30, 120);
  SetRateProfilePars(&rate_profile, 2, 200, 30, 240);
  rate_profile.frame_index_rate_update[3] = kNbrFramesLong + 1;
  rate_profile.num_frames = kNbrFramesLong;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.0f, 120, 1, false, true, true, true);
  
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 29.0, 20.0, 0.75, 0.60);
  
  RateControlMetrics rc_metrics[3];
  SetRateControlMetrics(rc_metrics, 0, 45, 30, 75, 20, 70, 0);
  SetRateControlMetrics(rc_metrics, 1, 20, 35, 30, 20, 15, 1);
  SetRateControlMetrics(rc_metrics, 2, 0, 30, 30, 15, 25, 1);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}






TEST_F(VideoProcessorIntegrationTest,
       DISABLED_ON_ANDROID(ProcessNoLossTemporalLayers)) {
  config_.networking_config.packet_loss_probability = 0;
  
  RateProfile rate_profile;
  SetRateProfilePars(&rate_profile, 0, 200, 30, 0);
  SetRateProfilePars(&rate_profile, 1, 400, 30, 150);
  rate_profile.frame_index_rate_update[2] = kNbrFramesLong + 1;
  rate_profile.num_frames = kNbrFramesLong;
  
  CodecConfigPars process_settings;
  SetCodecParameters(&process_settings, 0.0f, -1, 3, false, true, true, false);
  
  QualityMetrics quality_metrics;
  SetQualityMetrics(&quality_metrics, 32.5, 30.0, 0.85, 0.80);
  
  RateControlMetrics rc_metrics[2];
  SetRateControlMetrics(rc_metrics, 0, 0, 20, 30, 10, 10, 0);
  SetRateControlMetrics(rc_metrics, 1, 0, 0, 30, 15, 10, 0);
  ProcessFramesAndVerify(quality_metrics,
                         rate_profile,
                         process_settings,
                         rc_metrics);
}
}  
