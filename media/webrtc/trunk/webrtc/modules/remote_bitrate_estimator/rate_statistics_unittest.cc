









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/remote_bitrate_estimator/rate_statistics.h"

namespace {

using webrtc::RateStatistics;

class RateStatisticsTest : public ::testing::Test {
 protected:
  RateStatisticsTest() : stats_(500, 8000) {}
  RateStatistics stats_;
};

TEST_F(RateStatisticsTest, TestStrictMode) {
  int64_t now_ms = 0;
  
  EXPECT_EQ(0u, stats_.Rate(now_ms));
  stats_.Update(1500, now_ms);
  
  EXPECT_EQ(24000u, stats_.Rate(now_ms));
  stats_.Reset();
  
  EXPECT_EQ(0u, stats_.Rate(now_ms));
  for (int i = 0; i < 100000; ++i) {
    if (now_ms % 10 == 0) {
      stats_.Update(1500, now_ms);
    }
    
    
    if (now_ms > 0 && now_ms % 500 == 0) {
      EXPECT_NEAR(1200000u, stats_.Rate(now_ms), 24000u);
    }
    now_ms += 1;
  }
  now_ms += 500;
  
  
  EXPECT_EQ(0u, stats_.Rate(now_ms));
}

TEST_F(RateStatisticsTest, IncreasingThenDecreasingBitrate) {
  int64_t now_ms = 0;
  stats_.Reset();
  
  uint32_t bitrate = stats_.Rate(now_ms);
  EXPECT_EQ(0u, bitrate);
  
  while (++now_ms < 10000) {
    stats_.Update(1000, now_ms);
    uint32_t new_bitrate = stats_.Rate(now_ms);
    if (new_bitrate != bitrate) {
      
      EXPECT_GT(new_bitrate, bitrate);
    } else {
      
      EXPECT_NEAR(8000000u, bitrate, 80000u);
      break;
    }
    bitrate = new_bitrate;
  }
  
  while (++now_ms < 10000) {
    stats_.Update(1000, now_ms);
    bitrate = stats_.Rate(now_ms);
    EXPECT_NEAR(8000000u, bitrate, 80000u);
  }
  
  while (++now_ms < 20000) {
    stats_.Update(0, now_ms);
    uint32_t new_bitrate = stats_.Rate(now_ms);
    if (new_bitrate != bitrate) {
      
      EXPECT_LT(new_bitrate, bitrate);
    } else {
      
      EXPECT_EQ(0u, bitrate);
      break;
    }
    bitrate = new_bitrate;
  }
  
  while (++now_ms < 20000) {
    stats_.Update(0, now_ms);
    EXPECT_EQ(0u, stats_.Rate(now_ms));
  }
}
}  
