









#include "webrtc/base/gunit.h"
#include "webrtc/base/ratelimiter.h"

namespace rtc {

TEST(RateLimiterTest, TestCanUse) {
  
  RateLimiter limiter = RateLimiter(2000, 1.0);

  double monday = 1.0;
  double tuesday = 2.0;
  double thursday = 4.0;

  EXPECT_TRUE(limiter.CanUse(0, monday));
  EXPECT_TRUE(limiter.CanUse(1000, monday));
  EXPECT_TRUE(limiter.CanUse(1999, monday));
  EXPECT_TRUE(limiter.CanUse(2000, monday));
  EXPECT_FALSE(limiter.CanUse(2001, monday));

  limiter.Use(1000, monday);

  EXPECT_TRUE(limiter.CanUse(0, monday));
  EXPECT_TRUE(limiter.CanUse(999, monday));
  EXPECT_TRUE(limiter.CanUse(1000, monday));
  EXPECT_FALSE(limiter.CanUse(1001, monday));

  limiter.Use(1000, monday);

  EXPECT_TRUE(limiter.CanUse(0, monday));
  EXPECT_FALSE(limiter.CanUse(1, monday));

  EXPECT_TRUE(limiter.CanUse(0, tuesday));
  EXPECT_TRUE(limiter.CanUse(1, tuesday));
  EXPECT_TRUE(limiter.CanUse(1999, tuesday));
  EXPECT_TRUE(limiter.CanUse(2000, tuesday));
  EXPECT_FALSE(limiter.CanUse(2001, tuesday));

  limiter.Use(1000, tuesday);

  EXPECT_TRUE(limiter.CanUse(1000, tuesday));
  EXPECT_FALSE(limiter.CanUse(1001, tuesday));

  limiter.Use(1000, thursday);

  EXPECT_TRUE(limiter.CanUse(1000, tuesday));
  EXPECT_FALSE(limiter.CanUse(1001, tuesday));
}

}  
