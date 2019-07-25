









#ifndef WEBRTC_COMMONT_AUDIO_VAD_VAD_UNIT_TESTS_H
#define WEBRTC_COMMONT_AUDIO_VAD_VAD_UNIT_TESTS_H

#include <stddef.h>  

#include "gtest/gtest.h"
#include "typedefs.h"

namespace {


const int kModes[] = { 0, 1, 2, 3 };
const size_t kModesSize = sizeof(kModes) / sizeof(*kModes);


const int16_t kRates[] = { 8000, 12000, 16000, 24000, 32000 };
const size_t kRatesSize = sizeof(kRates) / sizeof(*kRates);


const int16_t kMaxFrameLength = 960;
const int16_t kFrameLengths[] = { 80, 120, 160, 240, 320, 480, 640,
    kMaxFrameLength };
const size_t kFrameLengthsSize = sizeof(kFrameLengths) / sizeof(*kFrameLengths);

}  

class VadTest : public ::testing::Test {
 protected:
  VadTest();
  virtual void SetUp();
  virtual void TearDown();

  
  bool ValidRatesAndFrameLengths(int16_t rate, int16_t frame_length);
};

#endif  
