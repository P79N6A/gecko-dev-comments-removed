











#include "webrtc/modules/audio_coding/neteq/expand.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/background_noise.h"
#include "webrtc/modules/audio_coding/neteq/random_vector.h"
#include "webrtc/modules/audio_coding/neteq/sync_buffer.h"

namespace webrtc {

TEST(Expand, CreateAndDestroy) {
  int fs = 8000;
  size_t channels = 1;
  BackgroundNoise bgn(channels);
  SyncBuffer sync_buffer(1, 1000);
  RandomVector random_vector;
  Expand expand(&bgn, &sync_buffer, &random_vector, fs, channels);
}

TEST(Expand, CreateUsingFactory) {
  int fs = 8000;
  size_t channels = 1;
  BackgroundNoise bgn(channels);
  SyncBuffer sync_buffer(1, 1000);
  RandomVector random_vector;
  ExpandFactory expand_factory;
  Expand* expand =
      expand_factory.Create(&bgn, &sync_buffer, &random_vector, fs, channels);
  EXPECT_TRUE(expand != NULL);
  delete expand;
}



}  
