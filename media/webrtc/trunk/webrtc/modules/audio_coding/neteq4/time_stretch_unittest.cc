











#include "webrtc/modules/audio_coding/neteq4/accelerate.h"
#include "webrtc/modules/audio_coding/neteq4/preemptive_expand.h"

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/background_noise.h"

namespace webrtc {

TEST(TimeStretch, CreateAndDestroy) {
  int sample_rate = 8000;
  size_t num_channels = 1;
  BackgroundNoise bgn(num_channels);
  Accelerate accelerate(sample_rate, num_channels, bgn);
  PreemptiveExpand preemptive_expand(sample_rate, num_channels, bgn);
}



}  
