





#include "base/platform_thread.h"
#include "base/spin_wait.h"
#include "base/time.h"
#include "base/watchdog.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;

namespace {




class WatchdogCounter : public Watchdog {
 public:
  WatchdogCounter(const TimeDelta& duration,
                  const std::string& thread_watched_name,
                  bool enabled)
      : Watchdog(duration, thread_watched_name, enabled), alarm_counter_(0) {
  }

  virtual ~WatchdogCounter() {}

  virtual void Alarm() {
    alarm_counter_++;
    Watchdog::Alarm();
  }

  int alarm_counter() { return alarm_counter_; }

 private:
  int alarm_counter_;

  DISALLOW_COPY_AND_ASSIGN(WatchdogCounter);
};

class WatchdogTest : public testing::Test {
};






TEST(WatchdogTest, StartupShutdownTest) {
  Watchdog watchdog1(TimeDelta::FromMilliseconds(300), "Disabled", false);
  Watchdog watchdog2(TimeDelta::FromMilliseconds(300), "Enabled", true);
}


TEST(WatchdogTest, ArmDisarmTest) {
  Watchdog watchdog1(TimeDelta::FromMilliseconds(300), "Disabled", false);
  watchdog1.Arm();
  watchdog1.Disarm();
  watchdog1.Arm();
  watchdog1.Disarm();

  Watchdog watchdog2(TimeDelta::FromMilliseconds(300), "Enabled", true);
  watchdog2.Arm();
  watchdog2.Disarm();
  watchdog2.Arm();
  watchdog2.Disarm();
}


TEST(WatchdogTest, AlarmTest) {
  WatchdogCounter watchdog(TimeDelta::FromMilliseconds(10), "Enabled", true);
  watchdog.Arm();
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(5),
                                   watchdog.alarm_counter() > 0);
  EXPECT_EQ(1, watchdog.alarm_counter());

  
  watchdog.ArmSomeTimeDeltaAgo(TimeDelta::FromSeconds(2));
  
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(5),
                                   watchdog.alarm_counter() > 1);

  EXPECT_EQ(2, watchdog.alarm_counter());
}


TEST(WatchdogTest, ConstructorDisabledTest) {
  WatchdogCounter watchdog(TimeDelta::FromMilliseconds(10), "Disabled", false);
  watchdog.Arm();
  
  PlatformThread::Sleep(500);
  EXPECT_EQ(0, watchdog.alarm_counter());
}


TEST(WatchdogTest, DisarmTest) {
  WatchdogCounter watchdog(TimeDelta::FromSeconds(5), "Enabled", true);
  watchdog.Arm();
  PlatformThread::Sleep(100);  
  watchdog.Disarm();
  
  PlatformThread::Sleep(5500);
  EXPECT_EQ(0, watchdog.alarm_counter());

  
  
  watchdog.ArmSomeTimeDeltaAgo(TimeDelta::FromSeconds(2));
  
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(5),
                                   watchdog.alarm_counter() > 0);

  EXPECT_EQ(1, watchdog.alarm_counter());
}

}  
