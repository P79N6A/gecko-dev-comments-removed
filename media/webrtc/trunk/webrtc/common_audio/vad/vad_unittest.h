









#ifndef WEBRTC_COMMON_AUDIO_VAD_VAD_UNITTEST_H
#define WEBRTC_COMMON_AUDIO_VAD_VAD_UNITTEST_H

#include <stddef.h>  

#include "gtest/gtest.h"

#include "typedefs.h"

namespace {


const int kModes[] = { 0, 1, 2, 3 };
const size_t kModesSize = sizeof(kModes) / sizeof(*kModes);


const int kRates[] = { 8000, 12000, 16000, 24000, 32000, 48000 };
const size_t kRatesSize = sizeof(kRates) / sizeof(*kRates);


const int kMaxFrameLength = 1440;
const int kFrameLengths[] = { 80, 120, 160, 240, 320, 480, 640, 960,
    kMaxFrameLength };
const size_t kFrameLengthsSize = sizeof(kFrameLengths) / sizeof(*kFrameLengths);

}  

class VadTest : public ::testing::Test {
 protected:
  VadTest();
  virtual void SetUp();
  virtual void TearDown();

  
  bool ValidRatesAndFrameLengths(int rate, int frame_length);
};

#endif  
