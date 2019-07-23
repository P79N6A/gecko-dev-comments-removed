



#include "base/system_monitor.h"
#include "testing/gtest/include/gtest/gtest.h"

class PowerTest : public base::SystemMonitor::PowerObserver {
 public:
  PowerTest()
    : battery_(false),
      power_state_changes_(0),
      suspends_(0),
      resumes_(0) {};

  
  void OnPowerStateChange(base::SystemMonitor*) { power_state_changes_++; };
  void OnSuspend(base::SystemMonitor*) { suspends_++; };
  void OnResume(base::SystemMonitor*) { resumes_++; };

  
  bool battery() { return battery_; }
  int power_state_changes() { return power_state_changes_; }
  int suspends() { return suspends_; }
  int resumes() { return resumes_; }

 private:
  bool battery_;   
  int power_state_changes_;  
  int suspends_;  
  int resumes_;  
};

TEST(SystemMonitor, PowerNotifications) {
  const int kObservers = 5;

  
  MessageLoop loop;
  
  base::Time now = base::Time::Now();

  base::SystemMonitor* monitor = base::SystemMonitor::Get();
  PowerTest test[kObservers];
  for (int index = 0; index < kObservers; ++index)
    monitor->AddObserver(&test[index]);

  
  
  for (int index = 0; index < 5; index++) {
    monitor->ProcessPowerMessage(base::SystemMonitor::POWER_STATE_EVENT);
    EXPECT_EQ(test[0].power_state_changes(), 0);
  }

  
  monitor->ProcessPowerMessage(base::SystemMonitor::RESUME_EVENT);
  loop.RunAllPending();
  EXPECT_EQ(test[0].resumes(), 0);

  
  monitor->ProcessPowerMessage(base::SystemMonitor::SUSPEND_EVENT);
  loop.RunAllPending();
  EXPECT_EQ(test[0].suspends(), 1);

  
  monitor->ProcessPowerMessage(base::SystemMonitor::SUSPEND_EVENT);
  loop.RunAllPending();
  EXPECT_EQ(test[0].suspends(), 1);

  
  monitor->ProcessPowerMessage(base::SystemMonitor::RESUME_EVENT);
  loop.RunAllPending();
  EXPECT_EQ(test[0].resumes(), 1);

  
  monitor->ProcessPowerMessage(base::SystemMonitor::RESUME_EVENT);
  loop.RunAllPending();
  EXPECT_EQ(test[0].resumes(), 1);
}
