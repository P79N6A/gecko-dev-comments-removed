









#include <math.h>

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/common_audio/resampler/include/resampler.h"



namespace webrtc {
namespace {
const ResamplerType kTypes[] = {
  kResamplerSynchronous,
  kResamplerSynchronousStereo,
};
const size_t kTypesSize = sizeof(kTypes) / sizeof(*kTypes);


const int kMaxRate = 96000;
const int kRates[] = {
  8000,
  16000,
  32000,
  44100,
  48000,
  kMaxRate
};
const size_t kRatesSize = sizeof(kRates) / sizeof(*kRates);
const int kMaxChannels = 2;
const size_t kDataSize = static_cast<size_t> (kMaxChannels * kMaxRate / 100);

class ResamplerTest : public testing::Test {
 protected:
  ResamplerTest();
  virtual void SetUp();
  virtual void TearDown();
  void RunResampleTest(int channels,
                       int src_sample_rate_hz,
                       int dst_sample_rate_hz);

  Resampler rs_;
  int16_t data_in_[kDataSize];
  int16_t data_out_[kDataSize];
  int16_t data_reference_[kDataSize];
};

ResamplerTest::ResamplerTest() {}

void ResamplerTest::SetUp() {
  
  memset(data_in_, 1, sizeof(data_in_));
}

void ResamplerTest::TearDown() {}

TEST_F(ResamplerTest, Reset) {
  
  
  

  
  for (size_t i = 0; i < kRatesSize; ++i) {
    for (size_t j = 0; j < kRatesSize; ++j) {
      for (size_t k = 0; k < kTypesSize; ++k) {
        std::ostringstream ss;
        ss << "Input rate: " << kRates[i] << ", output rate: " << kRates[j]
            << ", type: " << kTypes[k];
        SCOPED_TRACE(ss.str());
        EXPECT_EQ(0, rs_.Reset(kRates[i], kRates[j], kTypes[k]));
      }
    }
  }
}




void SetMonoFrame(int16_t* buffer, float data, int sample_rate_hz) {
  for (int i = 0; i < sample_rate_hz / 100; i++) {
    buffer[i] = data * i;
  }
}



void SetStereoFrame(int16_t* buffer, float left, float right,
                    int sample_rate_hz) {
  for (int i = 0; i < sample_rate_hz / 100; i++) {
    buffer[i * 2] = left * i;
    buffer[i * 2 + 1] = right * i;
  }
}




float ComputeSNR(const int16_t* reference, const int16_t* test,
                 int sample_rate_hz, int channels, int max_delay) {
  float best_snr = 0;
  int best_delay = 0;
  int samples_per_channel = sample_rate_hz/100;
  for (int delay = 0; delay < max_delay; delay++) {
    float mse = 0;
    float variance = 0;
    for (int i = 0; i < samples_per_channel * channels - delay; i++) {
      int error = reference[i] - test[i + delay];
      mse += error * error;
      variance += reference[i] * reference[i];
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

void ResamplerTest::RunResampleTest(int channels,
                                    int src_sample_rate_hz,
                                    int dst_sample_rate_hz) {
  Resampler resampler;  
  const int16_t kSrcLeft = 60;  
  const int16_t kSrcRight = 30;
  const float kResamplingFactor = (1.0 * src_sample_rate_hz) /
      dst_sample_rate_hz;
  const float kDstLeft = kResamplingFactor * kSrcLeft;
  const float kDstRight = kResamplingFactor * kSrcRight;
  if (channels == 1)
    SetMonoFrame(data_in_, kSrcLeft, src_sample_rate_hz);
  else
    SetStereoFrame(data_in_, kSrcLeft, kSrcRight, src_sample_rate_hz);

  if (channels == 1) {
    SetMonoFrame(data_out_, 0, dst_sample_rate_hz);
    SetMonoFrame(data_reference_, kDstLeft, dst_sample_rate_hz);
  } else {
    SetStereoFrame(data_out_, 0, 0, dst_sample_rate_hz);
    SetStereoFrame(data_reference_, kDstLeft, kDstRight, dst_sample_rate_hz);
  }

  
  
  
  
  
  static const int kInputKernelDelaySamples = 16*3;
  const int max_delay = std::min(1.0f, 1/kResamplingFactor) *
                        kInputKernelDelaySamples * channels * 2;
  printf("(%d, %d Hz) -> (%d, %d Hz) ",  
      channels, src_sample_rate_hz, channels, dst_sample_rate_hz);

  int in_length = channels * src_sample_rate_hz / 100;
  int out_length = 0;
  EXPECT_EQ(0, rs_.Reset(src_sample_rate_hz, dst_sample_rate_hz,
                         (channels == 1 ?
                          kResamplerSynchronous :
                          kResamplerSynchronousStereo)));
  EXPECT_EQ(0, rs_.Push(data_in_, in_length, data_out_, kDataSize,
                        out_length));
  EXPECT_EQ(channels * dst_sample_rate_hz / 100, out_length);

  
  EXPECT_GT(ComputeSNR(data_reference_, data_out_, dst_sample_rate_hz,
                       channels, max_delay), 40.0f);
}

TEST_F(ResamplerTest, Synchronous) {
  
  const int kChannels = 1;
  
  
  
  const int kSampleRates[] = {16000, 32000, 44100, 48000};
  const int kSampleRatesSize = sizeof(kSampleRates) / sizeof(*kSampleRates);
  for (int src_rate = 0; src_rate < kSampleRatesSize; src_rate++) {
    for (int dst_rate = 0; dst_rate < kSampleRatesSize; dst_rate++) {
      RunResampleTest(kChannels, kSampleRates[src_rate], kSampleRates[dst_rate]);
    }
  }
}

TEST_F(ResamplerTest, SynchronousStereo) {
  
  const int kChannels = 2;
  
  
  
  const int kSampleRates[] = {16000, 32000, 44100, 48000};
  const int kSampleRatesSize = sizeof(kSampleRates) / sizeof(*kSampleRates);
  for (int src_rate = 0; src_rate < kSampleRatesSize; src_rate++) {
    for (int dst_rate = 0; dst_rate < kSampleRatesSize; dst_rate++) {
      RunResampleTest(kChannels, kSampleRates[src_rate], kSampleRates[dst_rate]);
    }
  }
}
}  
}  
