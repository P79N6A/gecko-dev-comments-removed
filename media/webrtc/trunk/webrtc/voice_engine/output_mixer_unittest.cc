









#include <math.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/voice_engine/output_mixer.h"
#include "webrtc/voice_engine/output_mixer_internal.h"

namespace webrtc {
namespace voe {
namespace {

class OutputMixerTest : public ::testing::Test {
 protected:
  OutputMixerTest() {
    src_frame_.sample_rate_hz_ = 16000;
    src_frame_.samples_per_channel_ = src_frame_.sample_rate_hz_ / 100;
    src_frame_.num_channels_ = 1;
    dst_frame_.CopyFrom(src_frame_);
    golden_frame_.CopyFrom(src_frame_);
  }

  void RunResampleTest(int src_channels, int src_sample_rate_hz,
                       int dst_channels, int dst_sample_rate_hz);

  PushResampler resampler_;
  AudioFrame src_frame_;
  AudioFrame dst_frame_;
  AudioFrame golden_frame_;
};




void SetMonoFrame(AudioFrame* frame, float data, int sample_rate_hz) {
  memset(frame->data_, 0, sizeof(frame->data_));
  frame->num_channels_ = 1;
  frame->sample_rate_hz_ = sample_rate_hz;
  frame->samples_per_channel_ = sample_rate_hz / 100;
  for (int i = 0; i < frame->samples_per_channel_; i++) {
    frame->data_[i] = data * i;
  }
}


void SetMonoFrame(AudioFrame* frame, float data) {
  SetMonoFrame(frame, data, frame->sample_rate_hz_);
}



void SetStereoFrame(AudioFrame* frame, float left, float right,
                    int sample_rate_hz) {
  memset(frame->data_, 0, sizeof(frame->data_));
  frame->num_channels_ = 2;
  frame->sample_rate_hz_ = sample_rate_hz;
  frame->samples_per_channel_ = sample_rate_hz / 100;
  for (int i = 0; i < frame->samples_per_channel_; i++) {
    frame->data_[i * 2] = left * i;
    frame->data_[i * 2 + 1] = right * i;
  }
}


void SetStereoFrame(AudioFrame* frame, float left, float right) {
  SetStereoFrame(frame, left, right, frame->sample_rate_hz_);
}

void VerifyParams(const AudioFrame& ref_frame, const AudioFrame& test_frame) {
  EXPECT_EQ(ref_frame.num_channels_, test_frame.num_channels_);
  EXPECT_EQ(ref_frame.samples_per_channel_, test_frame.samples_per_channel_);
  EXPECT_EQ(ref_frame.sample_rate_hz_, test_frame.sample_rate_hz_);
}




float ComputeSNR(const AudioFrame& ref_frame, const AudioFrame& test_frame,
                 int max_delay) {
  VerifyParams(ref_frame, test_frame);
  float best_snr = 0;
  int best_delay = 0;
  for (int delay = 0; delay <= max_delay; delay++) {
    float mse = 0;
    float variance = 0;
    for (int i = 0; i < ref_frame.samples_per_channel_ *
        ref_frame.num_channels_ - delay; i++) {
      int error = ref_frame.data_[i] - test_frame.data_[i + delay];
      mse += error * error;
      variance += ref_frame.data_[i] * ref_frame.data_[i];
    }
    float snr = 100;  
    if (mse > 0)
      snr = 10 * log10(variance / mse);
    if (snr > best_snr) {
      best_snr = snr;
      best_delay = delay;
    }
  }
  printf("SNR=%.1f dB at delay=%d\n", best_snr, best_delay);
  return best_snr;
}

void VerifyFramesAreEqual(const AudioFrame& ref_frame,
                          const AudioFrame& test_frame) {
  VerifyParams(ref_frame, test_frame);
  for (int i = 0; i < ref_frame.samples_per_channel_ * ref_frame.num_channels_;
      i++) {
    EXPECT_EQ(ref_frame.data_[i], test_frame.data_[i]);
  }
}

void OutputMixerTest::RunResampleTest(int src_channels,
                                      int src_sample_rate_hz,
                                      int dst_channels,
                                      int dst_sample_rate_hz) {
  PushResampler resampler;  
  const int16_t kSrcLeft = 30;  
  const int16_t kSrcRight = 15;
  const float resampling_factor = (1.0 * src_sample_rate_hz) /
      dst_sample_rate_hz;
  const float dst_left = resampling_factor * kSrcLeft;
  const float dst_right = resampling_factor * kSrcRight;
  const float dst_mono = (dst_left + dst_right) / 2;
  if (src_channels == 1)
    SetMonoFrame(&src_frame_, kSrcLeft, src_sample_rate_hz);
  else
    SetStereoFrame(&src_frame_, kSrcLeft, kSrcRight, src_sample_rate_hz);

  if (dst_channels == 1) {
    SetMonoFrame(&dst_frame_, 0, dst_sample_rate_hz);
    if (src_channels == 1)
      SetMonoFrame(&golden_frame_, dst_left, dst_sample_rate_hz);
    else
      SetMonoFrame(&golden_frame_, dst_mono, dst_sample_rate_hz);
  } else {
    SetStereoFrame(&dst_frame_, 0, 0, dst_sample_rate_hz);
    if (src_channels == 1)
      SetStereoFrame(&golden_frame_, dst_left, dst_left, dst_sample_rate_hz);
    else
      SetStereoFrame(&golden_frame_, dst_left, dst_right, dst_sample_rate_hz);
  }

  
  
  
  
  
  static const int kInputKernelDelaySamples = 16*3;
  const int max_delay = std::min(1.0f, 1/kResamplingFactor) *
                        kInputKernelDelaySamples * dst_channels * 2;
  printf("(%d, %d Hz) -> (%d, %d Hz) ",  
      src_channels, src_sample_rate_hz, dst_channels, dst_sample_rate_hz);
  EXPECT_EQ(0, RemixAndResample(src_frame_, &resampler, &dst_frame_));
  if (src_sample_rate_hz == 96000 && dst_sample_rate_hz == 8000) {
    
    
    EXPECT_GT(ComputeSNR(golden_frame_, dst_frame_, max_delay), 14.0f);
  } else {
    EXPECT_GT(ComputeSNR(golden_frame_, dst_frame_, max_delay), 46.0f);
  }
}




TEST_F(OutputMixerTest, RemixAndResampleCopyFrameSucceeds) {
  
  SetStereoFrame(&src_frame_, 10, 10);
  SetStereoFrame(&dst_frame_, 0, 0);
  EXPECT_EQ(0, RemixAndResample(src_frame_, &resampler_, &dst_frame_));
  VerifyFramesAreEqual(src_frame_, dst_frame_);

  
  SetMonoFrame(&src_frame_, 20);
  SetMonoFrame(&dst_frame_, 0);
  EXPECT_EQ(0, RemixAndResample(src_frame_, &resampler_, &dst_frame_));
  VerifyFramesAreEqual(src_frame_, dst_frame_);
}

TEST_F(OutputMixerTest, RemixAndResampleMixingOnlySucceeds) {
  
  SetStereoFrame(&dst_frame_, 0, 0);
  SetMonoFrame(&src_frame_, 10);
  SetStereoFrame(&golden_frame_, 10, 10);
  EXPECT_EQ(0, RemixAndResample(src_frame_, &resampler_, &dst_frame_));
  VerifyFramesAreEqual(dst_frame_, golden_frame_);

  
  SetMonoFrame(&dst_frame_, 0);
  SetStereoFrame(&src_frame_, 10, 20);
  SetMonoFrame(&golden_frame_, 15);
  EXPECT_EQ(0, RemixAndResample(src_frame_, &resampler_, &dst_frame_));
  VerifyFramesAreEqual(golden_frame_, dst_frame_);
}

TEST_F(OutputMixerTest, RemixAndResampleSucceeds) {
  
  
  const int kSampleRates[] = {8000, 16000, 32000, 44100, 48000, 96000};
  const int kSampleRatesSize = sizeof(kSampleRates) / sizeof(*kSampleRates);
  const int kChannels[] = {1, 2};
  const int kChannelsSize = sizeof(kChannels) / sizeof(*kChannels);
  for (int src_rate = 0; src_rate < kSampleRatesSize; src_rate++) {
    for (int dst_rate = 0; dst_rate < kSampleRatesSize; dst_rate++) {
      for (int src_channel = 0; src_channel < kChannelsSize; src_channel++) {
        for (int dst_channel = 0; dst_channel < kChannelsSize; dst_channel++) {
          RunResampleTest(kChannels[src_channel], kSampleRates[src_rate],
                          kChannels[dst_channel], kSampleRates[dst_rate]);
        }
      }
    }
  }
}

}  
}  
}  
