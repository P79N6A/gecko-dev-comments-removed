



#include <time.h>

#include "base/platform_thread.h"
#include "base/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;


TEST(Time, TimeT) {
  
  time_t now_t_1 = time(NULL);
  struct tm tms;
#if defined(OS_WIN)
  localtime_s(&tms, &now_t_1);
#elif defined(OS_POSIX)
  localtime_r(&now_t_1, &tms);
#endif

  
  Time our_time_1 = Time::FromTimeT(now_t_1);
  Time::Exploded exploded;
  our_time_1.LocalExplode(&exploded);

  
  EXPECT_EQ(tms.tm_year + 1900, exploded.year);
  EXPECT_EQ(tms.tm_mon + 1, exploded.month);
  EXPECT_EQ(tms.tm_mday, exploded.day_of_month);
  EXPECT_EQ(tms.tm_hour, exploded.hour);
  EXPECT_EQ(tms.tm_min, exploded.minute);
  EXPECT_EQ(tms.tm_sec, exploded.second);

  
  Time our_time_2 = Time::FromLocalExploded(exploded);
  EXPECT_TRUE(our_time_1 == our_time_2);

  time_t now_t_2 = our_time_2.ToTimeT();
  EXPECT_EQ(now_t_1, now_t_2);

  EXPECT_EQ(10, Time().FromTimeT(10).ToTimeT());
  EXPECT_EQ(10.0, Time().FromTimeT(10).ToDoubleT());

  
  EXPECT_EQ(0, Time().ToTimeT());
  EXPECT_EQ(0, Time::FromTimeT(0).ToInternalValue());
}

TEST(Time, ZeroIsSymmetric) {
  Time zero_time(Time::FromTimeT(0));
  EXPECT_EQ(0, zero_time.ToTimeT());

  EXPECT_EQ(0.0, zero_time.ToDoubleT());
}

TEST(Time, LocalExplode) {
  Time a = Time::Now();
  Time::Exploded exploded;
  a.LocalExplode(&exploded);

  Time b = Time::FromLocalExploded(exploded);

  
  
  EXPECT_TRUE((a - b) < TimeDelta::FromMilliseconds(1));
}

TEST(Time, UTCExplode) {
  Time a = Time::Now();
  Time::Exploded exploded;
  a.UTCExplode(&exploded);

  Time b = Time::FromUTCExploded(exploded);
  EXPECT_TRUE((a - b) < TimeDelta::FromMilliseconds(1));
}

TEST(Time, LocalMidnight) {
  Time::Exploded exploded;
  Time::Now().LocalMidnight().LocalExplode(&exploded);
  EXPECT_EQ(0, exploded.hour);
  EXPECT_EQ(0, exploded.minute);
  EXPECT_EQ(0, exploded.second);
  EXPECT_EQ(0, exploded.millisecond);
}

TEST(TimeTicks, Deltas) {
  for (int index = 0; index < 500; index++) {
    TimeTicks ticks_start = TimeTicks::Now();
    PlatformThread::Sleep(10);
    TimeTicks ticks_stop = TimeTicks::Now();
    TimeDelta delta = ticks_stop - ticks_start;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    EXPECT_GE(delta.InMilliseconds(), 9);
    EXPECT_GE(delta.InMicroseconds(), 9000);
    EXPECT_EQ(delta.InSeconds(), 0);
  }
}

TEST(TimeTicks, HighResNow) {
  TimeTicks ticks_start = TimeTicks::HighResNow();
  PlatformThread::Sleep(10);
  TimeTicks ticks_stop = TimeTicks::HighResNow();
  TimeDelta delta = ticks_stop - ticks_start;
  EXPECT_GE(delta.InMicroseconds(), 9000);
}

TEST(TimeDelta, FromAndIn) {
  EXPECT_TRUE(TimeDelta::FromDays(2) == TimeDelta::FromHours(48));
  EXPECT_TRUE(TimeDelta::FromHours(3) == TimeDelta::FromMinutes(180));
  EXPECT_TRUE(TimeDelta::FromMinutes(2) == TimeDelta::FromSeconds(120));
  EXPECT_TRUE(TimeDelta::FromSeconds(2) == TimeDelta::FromMilliseconds(2000));
  EXPECT_TRUE(TimeDelta::FromMilliseconds(2) ==
              TimeDelta::FromMicroseconds(2000));
  EXPECT_EQ(13, TimeDelta::FromDays(13).InDays());
  EXPECT_EQ(13, TimeDelta::FromHours(13).InHours());
  EXPECT_EQ(13, TimeDelta::FromMinutes(13).InMinutes());
  EXPECT_EQ(13, TimeDelta::FromSeconds(13).InSeconds());
  EXPECT_EQ(13.0, TimeDelta::FromSeconds(13).InSecondsF());
  EXPECT_EQ(13, TimeDelta::FromMilliseconds(13).InMilliseconds());
  EXPECT_EQ(13.0, TimeDelta::FromMilliseconds(13).InMillisecondsF());
  EXPECT_EQ(13, TimeDelta::FromMicroseconds(13).InMicroseconds());
}
