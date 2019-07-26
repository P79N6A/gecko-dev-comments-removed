











#include "webrtc/modules/audio_coding/neteq4/accelerate.h"
#include "webrtc/modules/audio_coding/neteq4/preemptive_expand.h"

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/background_noise.h"

namespace webrtc {

TEST(TimeStretch, CreateAndDestroy) {
  const int kSampleRate = 8000;
  const size_t kNumChannels = 1;
  BackgroundNoise bgn(kNumChannels);
  Accelerate accelerate(kSampleRate, kNumChannels, bgn);
  PreemptiveExpand preemptive_expand(kSampleRate, kNumChannels, bgn);
}

TEST(TimeStretch, CreateUsingFactory) {
  const int kSampleRate = 8000;
  const size_t kNumChannels = 1;
  BackgroundNoise bgn(kNumChannels);

  AccelerateFactory accelerate_factory;
  Accelerate* accelerate =
      accelerate_factory.Create(kSampleRate, kNumChannels, bgn);
  EXPECT_TRUE(accelerate != NULL);
  delete accelerate;

  PreemptiveExpandFactory preemptive_expand_factory;
  PreemptiveExpand* preemptive_expand =
      preemptive_expand_factory.Create(kSampleRate, kNumChannels, bgn);
  EXPECT_TRUE(preemptive_expand != NULL);
  delete preemptive_expand;
}



}  
