









#include <stdlib.h>

#include "gtest/gtest.h"
#include "typedefs.h"
#include "vad_unittest.h"

extern "C" {
#include "vad_core.h"
#include "vad_sp.h"
}

namespace {

TEST_F(VadTest, vad_sp) {
  VadInstT* self = reinterpret_cast<VadInstT*>(malloc(sizeof(VadInstT)));
  const int kMaxFrameLenSp = 960;  
  int16_t zeros[kMaxFrameLenSp] = { 0 };
  int32_t state[2] = { 0 };
  int16_t data_in[kMaxFrameLenSp];
  int16_t data_out[kMaxFrameLenSp];

  
  
  static const int16_t kReferenceMin[32] = {
      1600, 720, 509, 512, 532, 552, 570, 588,
       606, 624, 642, 659, 675, 691, 707, 723,
      1600, 544, 502, 522, 542, 561, 579, 597,
       615, 633, 651, 667, 683, 699, 715, 731
  };

  
  
  for (int16_t i = 0; i < kMaxFrameLenSp; ++i) {
    data_in[i] = (i * i);
  }
  
  WebRtcVad_Downsampling(zeros, data_out, state, kMaxFrameLenSp);
  EXPECT_EQ(0, state[0]);
  EXPECT_EQ(0, state[1]);
  for (int16_t i = 0; i < kMaxFrameLenSp / 2; ++i) {
    EXPECT_EQ(0, data_out[i]);
  }
  
  WebRtcVad_Downsampling(data_in, data_out, state, kMaxFrameLenSp);
  EXPECT_EQ(207, state[0]);
  EXPECT_EQ(2270, state[1]);

  ASSERT_EQ(0, WebRtcVad_InitCore(self));
  
  
  
  for (int16_t i = 0; i < 16; ++i) {
    int16_t value = 500 * (i + 1);
    for (int j = 0; j < kNumChannels; ++j) {
      
      EXPECT_EQ(kReferenceMin[i], WebRtcVad_FindMinimum(self, value, j));
      EXPECT_EQ(kReferenceMin[i + 16], WebRtcVad_FindMinimum(self, 12000, j));
    }
    self->frame_counter++;
  }

  free(self);
}
}  
