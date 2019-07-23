
















#ifndef BASE_WATCHDOG_H__
#define BASE_WATCHDOG_H__

#include <string>

#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/platform_thread.h"
#include "base/time.h"

class Watchdog {
 public:
  
  Watchdog(const base::TimeDelta& duration,
           const std::string& thread_watched_name,
           bool enabled);
  virtual ~Watchdog();

  
  void Arm();  
  void ArmSomeTimeDeltaAgo(const base::TimeDelta& time_delta);
  void ArmAtStartTime(const base::TimeTicks start_time);

  
  void Disarm();

  
  
  virtual void Alarm() {
    DLOG(INFO) << "Watchdog alarmed for " << thread_watched_name_;
  }

 private:
  class ThreadDelegate : public PlatformThread::Delegate {
   public:
    explicit ThreadDelegate(Watchdog* watchdog) : watchdog_(watchdog) {
    }
    virtual void ThreadMain();
   private:
    Watchdog* watchdog_;

    void SetThreadName() const;
  };

  enum State {ARMED, DISARMED, SHUTDOWN };

  bool init_successful_;

  Lock lock_;  
  ConditionVariable condition_variable_;
  State state_;
  const base::TimeDelta duration_;  
  const std::string thread_watched_name_;
  PlatformThreadHandle handle_;
  ThreadDelegate delegate_;  

  base::TimeTicks start_time_;  

  
  
  
  
  
  
  static Lock static_lock_;  
  
  static base::TimeTicks last_debugged_alarm_time_;
  
  static base::TimeDelta last_debugged_alarm_delay_;

  DISALLOW_COPY_AND_ASSIGN(Watchdog);
};

#endif  
