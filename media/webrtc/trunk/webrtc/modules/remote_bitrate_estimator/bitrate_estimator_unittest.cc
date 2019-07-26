













#include <gtest/gtest.h>

#include "typedefs.h"
#include "bitrate_estimator.h"

namespace {

using webrtc::BitRateStats;

class BitRateStatsTest : public ::testing::Test
{
protected:
    BitRateStatsTest() {};
    BitRateStats bitRate;
};

TEST_F(BitRateStatsTest, TestStrictMode)
{
    WebRtc_Word64 nowMs = 0;
    
    EXPECT_EQ(0u, bitRate.BitRate(nowMs));
    bitRate.Update(1500, nowMs);
    
    EXPECT_EQ(24000u, bitRate.BitRate(nowMs));
    bitRate.Init();
    
    EXPECT_EQ(0u, bitRate.BitRate(nowMs));
    for (int i = 0; i < 100000; ++i)
    {
        if (nowMs % 10 == 0)
            bitRate.Update(1500, nowMs);
        
        
        if (nowMs > 0 && nowMs % 500 == 0)
            EXPECT_NEAR(1200000u, bitRate.BitRate(nowMs), 24000u);
        nowMs += 1;
    }
    nowMs += 500;
    
    
    EXPECT_EQ(0u, bitRate.BitRate(nowMs));
}

}
