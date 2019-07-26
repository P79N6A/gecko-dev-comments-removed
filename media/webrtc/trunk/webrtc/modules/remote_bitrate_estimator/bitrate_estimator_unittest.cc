









#include <gtest/gtest.h>

#include "webrtc/modules/remote_bitrate_estimator/bitrate_estimator.h"

namespace {

using webrtc::BitRateStats;

class BitRateStatsTest : public ::testing::Test {
 protected:
  BitRateStatsTest() {};
  BitRateStats stats_;
};

TEST_F(BitRateStatsTest, TestStrictMode) {
  int64_t now_ms = 0;
  
  EXPECT_EQ(0u, stats_.BitRate(now_ms));
  stats_.Update(1500, now_ms);
  
  EXPECT_EQ(24000u, stats_.BitRate(now_ms));
  stats_.Init();
  
  EXPECT_EQ(0u, stats_.BitRate(now_ms));
  for (int i = 0; i < 100000; ++i) {
    if (now_ms % 10 == 0) {
      stats_.Update(1500, now_ms);
    }
    
    
    if (now_ms > 0 && now_ms % 500 == 0) {
      EXPECT_NEAR(1200000u, stats_.BitRate(now_ms), 24000u);
    }
    now_ms += 1;
  }
  now_ms += 500;
  
  
  EXPECT_EQ(0u, stats_.BitRate(now_ms));
}
}  
