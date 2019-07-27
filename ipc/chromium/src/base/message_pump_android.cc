



#include "base/message_pump_android.h"

#include <fcntl.h>
#include <math.h>

#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/platform_thread.h"

namespace mozilla {
bool ProcessNextEvent();
void NotifyEvent();
}

namespace base {

MessagePumpForUI::MessagePumpForUI()
  : state_(NULL)
  , pump(*this)
{
}

MessagePumpForUI::~MessagePumpForUI() {
}

MessagePumpAndroid::MessagePumpAndroid(MessagePumpForUI &aPump)
  : pump(aPump)
{
}

MessagePumpAndroid::~MessagePumpAndroid()
{
}

void MessagePumpForUI::Run(Delegate* delegate) {
  RunState state;
  state.delegate = delegate;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  
  
  
  
  
  state.more_work_is_plausible = true;

  RunState* previous_state = state_;
  state_ = &state;

  
  
  

  while (!state_->should_quit) {
    mozilla::ProcessNextEvent();
    if (work_scheduled) {
      work_scheduled = false;
      HandleDispatch();
    }
  }

  state_ = previous_state;
}

void MessagePumpForUI::HandleDispatch() {
  
  
  
  if (state_->should_quit)
    return;

  state_->more_work_is_plausible = false;

  if (state_->delegate->DoWork())
    state_->more_work_is_plausible = true;

  if (state_->should_quit)
    return;

  if (state_->delegate->DoDelayedWork(&delayed_work_time_))
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;

  
  
  if (state_->more_work_is_plausible)
    return;

  if (state_->delegate->DoIdleWork())
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;
}

void MessagePumpForUI::Quit() {
  if (state_) {
    state_->should_quit = true;
  } else {
    NOTREACHED() << "Quit called outside Run!";
  }
}

void MessagePumpForUI::ScheduleWork() {
  
  
  
  work_scheduled = true;
  mozilla::NotifyEvent();
}

void MessagePumpForUI::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  
  
  delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}

}  
