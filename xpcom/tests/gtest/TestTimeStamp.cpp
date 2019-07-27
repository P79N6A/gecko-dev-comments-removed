




#include "mozilla/TimeStamp.h"

#include "prinrval.h"
#include "prthread.h"

#include "gtest/gtest.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

TEST(TimeStamp, Main)
{
    TimeDuration td;
    EXPECT_TRUE(td.ToSeconds() == 0.0);
    EXPECT_TRUE(TimeDuration::FromSeconds(5).ToSeconds() == 5.0);
    EXPECT_TRUE(TimeDuration::FromMilliseconds(5000).ToSeconds() == 5.0);
    EXPECT_TRUE(TimeDuration::FromSeconds(1) < TimeDuration::FromSeconds(2));
    EXPECT_FALSE(TimeDuration::FromSeconds(1) < TimeDuration::FromSeconds(1));
    EXPECT_TRUE(TimeDuration::FromSeconds(2) > TimeDuration::FromSeconds(1));
    EXPECT_FALSE(TimeDuration::FromSeconds(1) > TimeDuration::FromSeconds(1));
    EXPECT_TRUE(TimeDuration::FromSeconds(1) <= TimeDuration::FromSeconds(2));
    EXPECT_TRUE(TimeDuration::FromSeconds(1) <= TimeDuration::FromSeconds(1));
    EXPECT_FALSE(TimeDuration::FromSeconds(2) <= TimeDuration::FromSeconds(1));
    EXPECT_TRUE(TimeDuration::FromSeconds(2) >= TimeDuration::FromSeconds(1));
    EXPECT_TRUE(TimeDuration::FromSeconds(1) >= TimeDuration::FromSeconds(1));
    EXPECT_FALSE(TimeDuration::FromSeconds(1) >= TimeDuration::FromSeconds(2));

    TimeStamp ts;
    EXPECT_TRUE(ts.IsNull());

    ts = TimeStamp::Now();
    EXPECT_TRUE(!ts.IsNull());
    EXPECT_TRUE((ts - ts).ToSeconds() == 0.0);

    PR_Sleep(PR_SecondsToInterval(2));

    TimeStamp ts2(TimeStamp::Now());
    EXPECT_TRUE(ts2 > ts);
    EXPECT_FALSE(ts > ts);
    EXPECT_TRUE(ts < ts2);
    EXPECT_FALSE(ts < ts);
    EXPECT_TRUE(ts <= ts2);
    EXPECT_TRUE(ts <= ts);
    EXPECT_FALSE(ts2 <= ts);
    EXPECT_TRUE(ts2 >= ts);
    EXPECT_TRUE(ts2 >= ts);
    EXPECT_FALSE(ts >= ts2);

    
    
    
    td = ts2 - ts;
    EXPECT_TRUE(td.ToSeconds() > 1.0);
    EXPECT_TRUE(td.ToSeconds() < 20.0);
    td = ts - ts2;
    EXPECT_TRUE(td.ToSeconds() < -1.0);
    EXPECT_TRUE(td.ToSeconds() > -20.0);

    double resolution = TimeDuration::Resolution().ToSecondsSigDigits();
    printf(" (platform timer resolution is ~%g s)\n", resolution);
    EXPECT_TRUE(0.000000001 < resolution);
    
    
}
