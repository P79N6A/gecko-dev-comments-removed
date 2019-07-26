









#include "vad_unittest.h"

#include <stdlib.h>

#include "gtest/gtest.h"

#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "common_audio/vad/include/webrtc_vad.h"
#include "typedefs.h"

VadTest::VadTest() {}

void VadTest::SetUp() {}

void VadTest::TearDown() {}


bool VadTest::ValidRatesAndFrameLengths(int rate, int frame_length) {
  if (rate == 8000) {
    if (frame_length == 80 || frame_length == 160 || frame_length == 240) {
      return true;
    }
    return false;
  } else if (rate == 16000) {
    if (frame_length == 160 || frame_length == 320 || frame_length == 480) {
      return true;
    }
    return false;
  }
  if (rate == 32000) {
    if (frame_length == 320 || frame_length == 640 || frame_length == 960) {
      return true;
    }
    return false;
  }

  return false;
}

namespace {

TEST_F(VadTest, ApiTest) {
  
  

  VadInst* handle = NULL;
  int16_t zeros[kMaxFrameLength] = { 0 };

  
  
  int16_t speech[kMaxFrameLength];
  for (int16_t i = 0; i < kMaxFrameLength; i++) {
    speech[i] = (i * i);
  }

  
  EXPECT_EQ(-1, WebRtcVad_Create(NULL));
  EXPECT_EQ(-1, WebRtcVad_Init(NULL));
  EXPECT_EQ(-1, WebRtcVad_Free(NULL));
  EXPECT_EQ(-1, WebRtcVad_set_mode(NULL, kModes[0]));
  EXPECT_EQ(-1, WebRtcVad_Process(NULL, kRates[0], speech, kFrameLengths[0]));

  
  ASSERT_EQ(0, WebRtcVad_Create(&handle));

  
  EXPECT_EQ(-1, WebRtcVad_Process(handle, kRates[0], speech, kFrameLengths[0]));
  EXPECT_EQ(-1, WebRtcVad_set_mode(handle, kModes[0]));

  
  ASSERT_EQ(0, WebRtcVad_Init(handle));

  
  
  EXPECT_EQ(-1, WebRtcVad_set_mode(handle,
                                   WebRtcSpl_MinValueW32(kModes,
                                                         kModesSize) - 1));
  EXPECT_EQ(-1, WebRtcVad_set_mode(handle,
                                   WebRtcSpl_MaxValueW32(kModes,
                                                         kModesSize) + 1));

  
  
  EXPECT_EQ(-1, WebRtcVad_Process(handle, kRates[0], NULL, kFrameLengths[0]));
  
  EXPECT_EQ(-1, WebRtcVad_Process(handle, 9999, speech, kFrameLengths[0]));
  
  EXPECT_EQ(0, WebRtcVad_Process(handle, kRates[0], zeros, kFrameLengths[0]));
  for (size_t k = 0; k < kModesSize; k++) {
    
    EXPECT_EQ(0, WebRtcVad_set_mode(handle, kModes[k]));
    
    for (size_t i = 0; i < kRatesSize; i++) {
      for (size_t j = 0; j < kFrameLengthsSize; j++) {
        if (ValidRatesAndFrameLengths(kRates[i], kFrameLengths[j])) {
          EXPECT_EQ(1, WebRtcVad_Process(handle,
                                         kRates[i],
                                         speech,
                                         kFrameLengths[j]));
        } else {
          EXPECT_EQ(-1, WebRtcVad_Process(handle,
                                          kRates[i],
                                          speech,
                                          kFrameLengths[j]));
        }
      }
    }
  }

  EXPECT_EQ(0, WebRtcVad_Free(handle));
}

TEST_F(VadTest, ValidRatesFrameLengths) {
  
  
  
  for (int16_t rate = -1; rate <= kRates[kRatesSize - 1] + 1; rate++) {
    for (int16_t frame_length = -1; frame_length <= kMaxFrameLength + 1;
        frame_length++) {
      if (ValidRatesAndFrameLengths(rate, frame_length)) {
        EXPECT_EQ(0, WebRtcVad_ValidRateAndFrameLength(rate, frame_length));
      } else {
        EXPECT_EQ(-1, WebRtcVad_ValidRateAndFrameLength(rate, frame_length));
      }
    }
  }
}



}  
