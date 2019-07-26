









#include <stdlib.h>

#include "gtest/gtest.h"
#include "typedefs.h"
#include "vad_unittest.h"

extern "C" {
#include "vad_core.h"
#include "vad_filterbank.h"
}

namespace {

enum { kNumValidFrameLengths = 3 };

TEST_F(VadTest, vad_filterbank) {
  VadInstT* self = reinterpret_cast<VadInstT*>(malloc(sizeof(VadInstT)));
  static const int16_t kReference[kNumValidFrameLengths] = { 48, 11, 11 };
  static const int16_t kFeatures[kNumValidFrameLengths * kNumChannels] = {
      1213, 759, 587, 462, 434, 272,
      1479, 1385, 1291, 1200, 1103, 1099,
      1732, 1692, 1681, 1629, 1436, 1436
  };
  static const int16_t kOffsetVector[kNumChannels] = {
      368, 368, 272, 176, 176, 176 };
  int16_t features[kNumChannels];

  
  
  int16_t speech[kMaxFrameLength];
  for (int16_t i = 0; i < kMaxFrameLength; ++i) {
    speech[i] = (i * i);
  }

  int frame_length_index = 0;
  ASSERT_EQ(0, WebRtcVad_InitCore(self));
  for (size_t j = 0; j < kFrameLengthsSize; ++j) {
    if (ValidRatesAndFrameLengths(8000, kFrameLengths[j])) {
      EXPECT_EQ(kReference[frame_length_index],
                WebRtcVad_CalculateFeatures(self, speech, kFrameLengths[j],
                                            features));
      for (int k = 0; k < kNumChannels; ++k) {
        EXPECT_EQ(kFeatures[k + frame_length_index * kNumChannels],
                  features[k]);
      }
      frame_length_index++;
    }
  }
  EXPECT_EQ(kNumValidFrameLengths, frame_length_index);

  
  memset(speech, 0, sizeof(speech));
  ASSERT_EQ(0, WebRtcVad_InitCore(self));
  for (size_t j = 0; j < kFrameLengthsSize; ++j) {
    if (ValidRatesAndFrameLengths(8000, kFrameLengths[j])) {
      EXPECT_EQ(0, WebRtcVad_CalculateFeatures(self, speech, kFrameLengths[j],
                                               features));
      for (int k = 0; k < kNumChannels; ++k) {
        EXPECT_EQ(kOffsetVector[k], features[k]);
      }
    }
  }

  
  
  for (int16_t i = 0; i < kMaxFrameLength; ++i) {
    speech[i] = 1;
  }
  for (size_t j = 0; j < kFrameLengthsSize; ++j) {
    if (ValidRatesAndFrameLengths(8000, kFrameLengths[j])) {
      ASSERT_EQ(0, WebRtcVad_InitCore(self));
      EXPECT_EQ(0, WebRtcVad_CalculateFeatures(self, speech, kFrameLengths[j],
                                               features));
      for (int k = 0; k < kNumChannels; ++k) {
        EXPECT_EQ(kOffsetVector[k], features[k]);
      }
    }
  }

  free(self);
}
}  
