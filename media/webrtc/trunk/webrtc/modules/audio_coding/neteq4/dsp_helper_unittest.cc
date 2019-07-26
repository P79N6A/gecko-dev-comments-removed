









#include "webrtc/modules/audio_coding/neteq4/dsp_helper.h"

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/typedefs.h"

namespace webrtc {

TEST(DspHelper, RampSignalArray) {
  static const int kLen = 100;
  int16_t input[kLen];
  int16_t output[kLen];
  
  for (int i = 0; i < kLen; ++i) {
    input[i] = 1000;
  }
  int start_factor = 0;
  
  
  int increment = (16384 << 6) / kLen;

  
  int stop_factor = DspHelper::RampSignal(input, kLen, start_factor, increment,
                                          output);
  EXPECT_EQ(16383, stop_factor);  
  for (int i = 0; i < kLen; ++i) {
    EXPECT_EQ(1000 * i / kLen, output[i]);
  }

  
  stop_factor = DspHelper::RampSignal(input, kLen, start_factor, increment);
  EXPECT_EQ(16383, stop_factor);  
  for (int i = 0; i < kLen; ++i) {
    EXPECT_EQ(1000 * i / kLen, input[i]);
  }
}

TEST(DspHelper, RampSignalAudioMultiVector) {
  static const int kLen = 100;
  static const int kChannels = 5;
  AudioMultiVector input(kChannels, kLen * 3);
  
  for (int i = 0; i < kLen * 3; ++i) {
    for (int channel = 0; channel < kChannels; ++channel) {
      input[channel][i] = 1000;
    }
  }
  
  
  int start_index = kLen;
  int start_factor = 0;
  
  
  int increment = (16384 << 6) / kLen;

  int stop_factor = DspHelper::RampSignal(&input, start_index, kLen,
                                          start_factor, increment);
  EXPECT_EQ(16383, stop_factor);  
  
  int i;
  for (i = 0; i < kLen; ++i) {
    for (int channel = 0; channel < kChannels; ++channel) {
      EXPECT_EQ(1000, input[channel][i]);
    }
  }
  
  for (; i < 2 * kLen; ++i) {
    for (int channel = 0; channel < kChannels; ++channel) {
      EXPECT_EQ(1000 * (i - kLen) / kLen, input[channel][i]);
    }
  }
  
  for (; i < 3 * kLen; ++i) {
    for (int channel = 0; channel < kChannels; ++channel) {
      EXPECT_EQ(1000, input[channel][i]);
    }
  }
}
}  
