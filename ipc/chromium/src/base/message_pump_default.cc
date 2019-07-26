



#include "base/message_pump_default.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/scoped_nsautorelease_pool.h"
#include "GeckoProfiler.h"

#include "mozilla/BackgroundHangMonitor.h"

namespace base {

MessagePumpDefault::MessagePumpDefault()
    : keep_running_(true),
      event_(false, false) {
}

void MessagePumpDefault::Run(Delegate* delegate) {
  DCHECK(keep_running_) << "Quit must have been called outside of Run!";

  const MessageLoop* const loop = MessageLoop::current();
  mozilla::BackgroundHangMonitor hangMonitor(
    loop->thread_name().c_str(),
    loop->transient_hang_timeout(),
    loop->permanent_hang_timeout());

  for (;;) {
    ScopedNSAutoreleasePool autorelease_pool;

    hangMonitor.NotifyActivity();
    bool did_work = delegate->DoWork();
    if (!keep_running_)
      break;

    hangMonitor.NotifyActivity();
    did_work |= delegate->DoDelayedWork(&delayed_work_time_);
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    hangMonitor.NotifyActivity();
    did_work = delegate->DoIdleWork();
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    if (delayed_work_time_.is_null()) {
      hangMonitor.NotifyWait();
      PROFILER_LABEL("MessagePump", "Wait",
        js::ProfileEntry::Category::OTHER);
      {
        GeckoProfilerSleepRAII profiler_sleep;
        event_.Wait();
      }
    } else {
      TimeDelta delay = delayed_work_time_ - TimeTicks::Now();
      if (delay > TimeDelta()) {
        hangMonitor.NotifyWait();
        PROFILER_LABEL("MessagePump", "Wait",
          js::ProfileEntry::Category::OTHER);
        {
          GeckoProfilerSleepRAII profiler_sleep;
          event_.TimedWait(delay);
        }
      } else {
        
        
        delayed_work_time_ = TimeTicks();
      }
    }
    
    
  }

  keep_running_ = true;
}

void MessagePumpDefault::Quit() {
  keep_running_ = false;
}

void MessagePumpDefault::ScheduleWork() {
  
  
  event_.Signal();
}

void MessagePumpDefault::ScheduleDelayedWork(
    const TimeTicks& delayed_work_time) {
  
  
  
  delayed_work_time_ = delayed_work_time;
}

}  
