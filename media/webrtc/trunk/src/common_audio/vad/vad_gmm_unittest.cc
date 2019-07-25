









#include "gtest/gtest.h"
#include "typedefs.h"
#include "vad_unittest.h"

extern "C" {
#include "vad_gmm.h"
}

namespace {

TEST_F(VadTest, vad_gmm) {
  int16_t delta = 0;
  
  EXPECT_EQ(1048576, WebRtcVad_GaussianProbability(0, 0, 128, &delta));
  EXPECT_EQ(0, delta);
  EXPECT_EQ(1048576, WebRtcVad_GaussianProbability(16, 128, 128, &delta));
  EXPECT_EQ(0, delta);
  EXPECT_EQ(1048576, WebRtcVad_GaussianProbability(-16, -128, 128, &delta));
  EXPECT_EQ(0, delta);

  
  EXPECT_EQ(1024, WebRtcVad_GaussianProbability(59, 0, 128, &delta));
  EXPECT_EQ(7552, delta);
  EXPECT_EQ(1024, WebRtcVad_GaussianProbability(75, 128, 128, &delta));
  EXPECT_EQ(7552, delta);
  EXPECT_EQ(1024, WebRtcVad_GaussianProbability(-75, -128, 128, &delta));
  EXPECT_EQ(-7552, delta);

  
  EXPECT_EQ(0, WebRtcVad_GaussianProbability(105, 0, 128, &delta));
  EXPECT_EQ(13440, delta);
}
}  
