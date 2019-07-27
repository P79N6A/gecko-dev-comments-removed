









#include <limits>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/pacing/bitrate_prober.h"

namespace webrtc {

TEST(BitrateProberTest, VerifyStatesAndTimeBetweenProbes) {
  BitrateProber prober;
  EXPECT_FALSE(prober.IsProbing());
  int64_t now_ms = 0;
  EXPECT_EQ(std::numeric_limits<int>::max(), prober.TimeUntilNextProbe(now_ms));

  prober.SetEnabled(true);
  EXPECT_FALSE(prober.IsProbing());

  prober.MaybeInitializeProbe(300000);
  EXPECT_TRUE(prober.IsProbing());

  EXPECT_EQ(0, prober.TimeUntilNextProbe(now_ms));
  prober.PacketSent(now_ms, 1000);

  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(8, prober.TimeUntilNextProbe(now_ms));
    now_ms += 4;
    EXPECT_EQ(4, prober.TimeUntilNextProbe(now_ms));
    now_ms += 4;
    EXPECT_EQ(0, prober.TimeUntilNextProbe(now_ms));
    prober.PacketSent(now_ms, 1000);
  }
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(4, prober.TimeUntilNextProbe(now_ms));
    now_ms += 4;
    EXPECT_EQ(0, prober.TimeUntilNextProbe(now_ms));
    prober.PacketSent(now_ms, 1000);
  }

  EXPECT_EQ(std::numeric_limits<int>::max(), prober.TimeUntilNextProbe(now_ms));
  EXPECT_FALSE(prober.IsProbing());
}
}  
