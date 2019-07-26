









#include <stdlib.h>

#include "gtest/gtest.h"
#include "typedefs.h"
#include "vad_unittest.h"

extern "C" {
#include "vad_core.h"
}

namespace {

TEST_F(VadTest, InitCore) {
  
  VadInstT* self = reinterpret_cast<VadInstT*>(malloc(sizeof(VadInstT)));

  
  EXPECT_EQ(-1, WebRtcVad_InitCore(NULL));

  
  EXPECT_EQ(0, WebRtcVad_InitCore(self));
  
  EXPECT_EQ(42, self->init_flag);

  free(self);
}

TEST_F(VadTest, set_mode_core) {
  VadInstT* self = reinterpret_cast<VadInstT*>(malloc(sizeof(VadInstT)));

  
  

  ASSERT_EQ(0, WebRtcVad_InitCore(self));
  
  
  EXPECT_EQ(-1, WebRtcVad_set_mode_core(self, -1));
  EXPECT_EQ(-1, WebRtcVad_set_mode_core(self, 1000));
  
  for (size_t j = 0; j < kModesSize; ++j) {
    EXPECT_EQ(0, WebRtcVad_set_mode_core(self, kModes[j]));
  }

  free(self);
}

TEST_F(VadTest, CalcVad) {
  VadInstT* self = reinterpret_cast<VadInstT*>(malloc(sizeof(VadInstT)));
  int16_t speech[kMaxFrameLength];

  
  

  
  
  memset(speech, 0, sizeof(speech));
  ASSERT_EQ(0, WebRtcVad_InitCore(self));
  for (size_t j = 0; j < kFrameLengthsSize; ++j) {
    if (ValidRatesAndFrameLengths(8000, kFrameLengths[j])) {
      EXPECT_EQ(0, WebRtcVad_CalcVad8khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(16000, kFrameLengths[j])) {
      EXPECT_EQ(0, WebRtcVad_CalcVad16khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(32000, kFrameLengths[j])) {
      EXPECT_EQ(0, WebRtcVad_CalcVad32khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(48000, kFrameLengths[j])) {
      EXPECT_EQ(0, WebRtcVad_CalcVad48khz(self, speech, kFrameLengths[j]));
    }
  }

  
  
  for (int16_t i = 0; i < kMaxFrameLength; ++i) {
    speech[i] = (i * i);
  }
  for (size_t j = 0; j < kFrameLengthsSize; ++j) {
    if (ValidRatesAndFrameLengths(8000, kFrameLengths[j])) {
      EXPECT_EQ(1, WebRtcVad_CalcVad8khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(16000, kFrameLengths[j])) {
      EXPECT_EQ(1, WebRtcVad_CalcVad16khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(32000, kFrameLengths[j])) {
      EXPECT_EQ(1, WebRtcVad_CalcVad32khz(self, speech, kFrameLengths[j]));
    }
    if (ValidRatesAndFrameLengths(48000, kFrameLengths[j])) {
      EXPECT_EQ(1, WebRtcVad_CalcVad48khz(self, speech, kFrameLengths[j]));
    }
  }

  free(self);
}
}  
