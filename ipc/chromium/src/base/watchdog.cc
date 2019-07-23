



#include "base/watchdog.h"

#include "base/compiler_specific.h"
#include "base/platform_thread.h"

using base::TimeDelta;
using base::TimeTicks;





Watchdog::Watchdog(const TimeDelta& duration,
                   const std::string& thread_watched_name,
                   bool enabled)
  : init_successful_(false),
    lock_(),
    condition_variable_(&lock_),
    state_(DISARMED),
    duration_(duration),
    thread_watched_name_(thread_watched_name),
    ALLOW_THIS_IN_INITIALIZER_LIST(delegate_(this)) {
  if (!enabled)
    return;  
  init_successful_ = PlatformThread::Create(0,  
                                            &delegate_,
                                            &handle_);
  DCHECK(init_successful_);
}


Watchdog::~Watchdog() {
  if (!init_successful_)
    return;
  {
    AutoLock lock(lock_);
    state_ = SHUTDOWN;
  }
  condition_variable_.Signal();
  PlatformThread::Join(handle_);
}

void Watchdog::Arm() {
  ArmAtStartTime(TimeTicks::Now());
}

void Watchdog::ArmSomeTimeDeltaAgo(const TimeDelta& time_delta) {
  ArmAtStartTime(TimeTicks::Now() - time_delta);
}


void Watchdog::ArmAtStartTime(const TimeTicks start_time) {
  {
    AutoLock lock(lock_);
    start_time_ = start_time;
    state_ = ARMED;
  }
  
  
  condition_variable_.Signal();
}


void Watchdog::Disarm() {
  AutoLock lock(lock_);
  state_ = DISARMED;
  
  
}




void Watchdog::ThreadDelegate::ThreadMain() {
  SetThreadName();
  TimeDelta remaining_duration;
  while (1) {
    AutoLock lock(watchdog_->lock_);
    while (DISARMED == watchdog_->state_)
      watchdog_->condition_variable_.Wait();
    if (SHUTDOWN == watchdog_->state_)
      return;
    DCHECK(ARMED == watchdog_->state_);
    remaining_duration = watchdog_->duration_ -
        (TimeTicks::Now() - watchdog_->start_time_);
    if (remaining_duration.InMilliseconds() > 0) {
      
      watchdog_->condition_variable_.TimedWait(remaining_duration);
    } else {
      
      
      {
        AutoLock static_lock(static_lock_);
        if (last_debugged_alarm_time_ > watchdog_->start_time_) {
          
          
          watchdog_->start_time_ += last_debugged_alarm_delay_;
          if (last_debugged_alarm_time_ > watchdog_->start_time_)
            
            watchdog_->state_ = DISARMED;
          continue;
        }
      }
      watchdog_->state_ = DISARMED;  
      TimeTicks last_alarm_time = TimeTicks::Now();
      watchdog_->Alarm();  
      TimeDelta last_alarm_delay = TimeTicks::Now() - last_alarm_time;
      if (last_alarm_delay > TimeDelta::FromMilliseconds(2)) {
        
        AutoLock static_lock(static_lock_);
        
        last_debugged_alarm_time_ = last_alarm_time;
        last_debugged_alarm_delay_ = last_alarm_delay;
      }
    }
  }
}

void Watchdog::ThreadDelegate::SetThreadName() const {
  std::string name = watchdog_->thread_watched_name_ + " Watchdog";
  PlatformThread::SetName(name.c_str());
  DLOG(INFO) << "Watchdog active: " << name;
}


Lock Watchdog::static_lock_;  

TimeTicks Watchdog::last_debugged_alarm_time_ = TimeTicks();

TimeDelta Watchdog::last_debugged_alarm_delay_;
