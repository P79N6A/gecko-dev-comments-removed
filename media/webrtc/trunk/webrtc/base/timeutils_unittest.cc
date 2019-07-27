









#include "webrtc/base/common.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

namespace rtc {

TEST(TimeTest, TimeInMs) {
  uint32 ts_earlier = Time();
  Thread::SleepMs(100);
  uint32 ts_now = Time();
  
  EXPECT_GE(ts_now, ts_earlier + 80);
  
  EXPECT_LT(ts_now, ts_earlier + 1000);
}

TEST(TimeTest, Comparison) {
  
  TimeStamp ts_earlier = Time();
  Thread::SleepMs(100);
  TimeStamp ts_now = Time();
  EXPECT_NE(ts_earlier, ts_now);

  
  EXPECT_TRUE( TimeIsLaterOrEqual(ts_earlier, ts_now));
  EXPECT_TRUE( TimeIsLater(       ts_earlier, ts_now));
  EXPECT_FALSE(TimeIsLaterOrEqual(ts_now,     ts_earlier));
  EXPECT_FALSE(TimeIsLater(       ts_now,     ts_earlier));

  
  EXPECT_TRUE( TimeIsLaterOrEqual(ts_earlier, ts_earlier));
  EXPECT_FALSE(TimeIsLater(       ts_earlier, ts_earlier));

  
  TimeStamp ts_later = TimeAfter(100);
  EXPECT_NE(ts_now, ts_later);
  EXPECT_TRUE( TimeIsLater(ts_now,     ts_later));
  EXPECT_TRUE( TimeIsLater(ts_earlier, ts_later));

  
  EXPECT_TRUE( TimeIsBetween(ts_earlier, ts_now,     ts_later));
  EXPECT_FALSE(TimeIsBetween(ts_earlier, ts_later,   ts_now));
  EXPECT_FALSE(TimeIsBetween(ts_now,     ts_earlier, ts_later));
  EXPECT_TRUE( TimeIsBetween(ts_now,     ts_later,   ts_earlier));
  EXPECT_TRUE( TimeIsBetween(ts_later,   ts_earlier, ts_now));
  EXPECT_FALSE(TimeIsBetween(ts_later,   ts_now,     ts_earlier));

  
  EXPECT_TRUE( TimeIsBetween(ts_earlier, ts_earlier, ts_earlier));
  EXPECT_TRUE( TimeIsBetween(ts_earlier, ts_earlier, ts_later));
  EXPECT_TRUE( TimeIsBetween(ts_earlier, ts_later,   ts_later));

  
  EXPECT_EQ(ts_earlier, TimeMin(ts_earlier, ts_earlier));
  EXPECT_EQ(ts_earlier, TimeMin(ts_earlier, ts_now));
  EXPECT_EQ(ts_earlier, TimeMin(ts_earlier, ts_later));
  EXPECT_EQ(ts_earlier, TimeMin(ts_now,     ts_earlier));
  EXPECT_EQ(ts_earlier, TimeMin(ts_later,   ts_earlier));

  
  EXPECT_EQ(ts_earlier, TimeMax(ts_earlier, ts_earlier));
  EXPECT_EQ(ts_now,     TimeMax(ts_earlier, ts_now));
  EXPECT_EQ(ts_later,   TimeMax(ts_earlier, ts_later));
  EXPECT_EQ(ts_now,     TimeMax(ts_now,     ts_earlier));
  EXPECT_EQ(ts_later,   TimeMax(ts_later,   ts_earlier));
}

TEST(TimeTest, Intervals) {
  TimeStamp ts_earlier = Time();
  TimeStamp ts_later = TimeAfter(500);

  
  
  EXPECT_LE(500,  TimeDiff(ts_later, ts_earlier));
  EXPECT_GE(-500, TimeDiff(ts_earlier, ts_later));

  
  EXPECT_GE(TimeSince(ts_earlier), 0);

  
  EXPECT_LE(TimeUntil(ts_earlier), 0);

  
  
  EXPECT_GE(TimeSince(ts_later), -500);

  
  EXPECT_LE(TimeUntil(ts_later), 500);
}

TEST(TimeTest, BoundaryComparison) {
  
  TimeStamp ts_earlier = static_cast<TimeStamp>(-50);
  TimeStamp ts_later = ts_earlier + 100;
  EXPECT_NE(ts_earlier, ts_later);

  
  EXPECT_TRUE( TimeIsLaterOrEqual(ts_earlier, ts_later));
  EXPECT_TRUE( TimeIsLater(       ts_earlier, ts_later));
  EXPECT_FALSE(TimeIsLaterOrEqual(ts_later,   ts_earlier));
  EXPECT_FALSE(TimeIsLater(       ts_later,   ts_earlier));

  
  EXPECT_EQ(ts_earlier, TimeMin(ts_earlier, ts_earlier));
  EXPECT_EQ(ts_earlier, TimeMin(ts_earlier, ts_later));
  EXPECT_EQ(ts_earlier, TimeMin(ts_later,   ts_earlier));

  
  EXPECT_EQ(ts_earlier, TimeMax(ts_earlier, ts_earlier));
  EXPECT_EQ(ts_later,   TimeMax(ts_earlier, ts_later));
  EXPECT_EQ(ts_later,   TimeMax(ts_later,   ts_earlier));

  
  EXPECT_EQ(100,  TimeDiff(ts_later, ts_earlier));
  EXPECT_EQ(-100, TimeDiff(ts_earlier, ts_later));
}

TEST(TimeTest, DISABLED_CurrentTmTime) {
  struct tm tm;
  int microseconds;

  time_t before = ::time(NULL);
  CurrentTmTime(&tm, &microseconds);
  time_t after = ::time(NULL);

  
  
  time_t local_delta = before - ::mktime(::gmtime(&before));  
  time_t t = ::mktime(&tm) + local_delta;

  EXPECT_TRUE(before <= t && t <= after);
  EXPECT_TRUE(0 <= microseconds && microseconds < 1000000);
}

class TimestampWrapAroundHandlerTest : public testing::Test {
 public:
  TimestampWrapAroundHandlerTest() {}

 protected:
  TimestampWrapAroundHandler wraparound_handler_;
};

TEST_F(TimestampWrapAroundHandlerTest, Unwrap) {
  uint32 ts = 0xfffffff2;
  int64 unwrapped_ts = ts;
  EXPECT_EQ(ts, wraparound_handler_.Unwrap(ts));
  ts = 2;
  unwrapped_ts += 0x10;
  EXPECT_EQ(unwrapped_ts, wraparound_handler_.Unwrap(ts));
  ts = 0xfffffff2;
  unwrapped_ts += 0xfffffff0;
  EXPECT_EQ(unwrapped_ts, wraparound_handler_.Unwrap(ts));
  ts = 0;
  unwrapped_ts += 0xe;
  EXPECT_EQ(unwrapped_ts, wraparound_handler_.Unwrap(ts));
}

}  
