











#include "webrtc/modules/audio_coding/neteq4/background_noise.h"

#include "gtest/gtest.h"

namespace webrtc {

TEST(BackgroundNoise, CreateAndDestroy) {
  size_t channels = 1;
  BackgroundNoise bgn(channels);
}



}  
