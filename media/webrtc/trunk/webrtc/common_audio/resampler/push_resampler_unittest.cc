









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_audio/resampler/include/push_resampler.h"



namespace webrtc {

TEST(PushResamplerTest, VerifiesInputParameters) {
  PushResampler resampler;
  EXPECT_EQ(-1, resampler.InitializeIfNeeded(-1, 16000, 1));
  EXPECT_EQ(-1, resampler.InitializeIfNeeded(16000, -1, 1));
  EXPECT_EQ(-1, resampler.InitializeIfNeeded(16000, 16000, 0));
  EXPECT_EQ(-1, resampler.InitializeIfNeeded(16000, 16000, 3));
  EXPECT_EQ(0, resampler.InitializeIfNeeded(16000, 16000, 1));
  EXPECT_EQ(0, resampler.InitializeIfNeeded(16000, 16000, 2));
}

}  
