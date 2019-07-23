


























#ifndef BASE_IDLE_TIMER_H_
#define BASE_IDLE_TIMER_H_

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/basictypes.h"
#include "base/task.h"
#include "base/timer.h"

namespace base {



typedef bool (*IdleTimeSource)(int32 *milliseconds_interval_since_last_event);

class IdleTimer {
 public:
  
  
  
  
  IdleTimer(TimeDelta idle_time, bool repeat);

  
  virtual ~IdleTimer();

  
  void Start();

  
  void Stop();

  
  virtual void OnIdle() = 0;

 protected:
  
  void set_idle_time_source(IdleTimeSource idle_time_source) {
    idle_time_source_ = idle_time_source;
  }

 private:
  
  void Run();

  
  void StartTimer();

  
  TimeDelta CurrentIdleTime();

  
  TimeDelta TimeUntilIdle();

  TimeDelta idle_interval_;
  bool repeat_;
  Time last_time_fired_;  
                          
  OneShotTimer<IdleTimer> timer_;

  IdleTimeSource idle_time_source_;

  DISALLOW_COPY_AND_ASSIGN(IdleTimer);
};

}  

#endif  
