



#include "base/message_pump_glib.h"

#include <fcntl.h>
#include <math.h>

#include "base/eintr_wrapper.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/platform_thread.h"

namespace {


const char kWorkScheduled = '\0';



int GetTimeIntervalMilliseconds(base::Time from) {
  if (from.is_null())
    return -1;

  
  
  
  int delay = static_cast<int>(
      ceil((from - base::Time::Now()).InMillisecondsF()));

  
  return delay < 0 ? 0 : delay;
}



















struct WorkSource : public GSource {
  base::MessagePumpForUI* pump;
};

gboolean WorkSourcePrepare(GSource* source,
                           gint* timeout_ms) {
  *timeout_ms = static_cast<WorkSource*>(source)->pump->HandlePrepare();
  
  
  
  return FALSE;
}

gboolean WorkSourceCheck(GSource* source) {
  
  return TRUE;
}

gboolean WorkSourceDispatch(GSource* source,
                            GSourceFunc unused_func,
                            gpointer unused_data) {

  static_cast<WorkSource*>(source)->pump->HandleDispatch();
  
  return TRUE;
}


GSourceFuncs WorkSourceFuncs = {
  WorkSourcePrepare,
  WorkSourceCheck,
  WorkSourceDispatch,
  NULL
};

}  


namespace base {

MessagePumpForUI::MessagePumpForUI()
    : state_(NULL),
      context_(g_main_context_default()) {
  
  int fds[2];
  CHECK(pipe(fds) == 0);
  wakeup_pipe_read_  = fds[0];
  wakeup_pipe_write_ = fds[1];
  wakeup_gpollfd_.fd = wakeup_pipe_read_;
  wakeup_gpollfd_.events = G_IO_IN;

  work_source_ = g_source_new(&WorkSourceFuncs, sizeof(WorkSource));
  static_cast<WorkSource*>(work_source_)->pump = this;
  g_source_add_poll(work_source_, &wakeup_gpollfd_);
  
  g_source_set_priority(work_source_, G_PRIORITY_DEFAULT_IDLE);
  
  g_source_set_can_recurse(work_source_, TRUE);
  g_source_attach(work_source_, context_);
}

MessagePumpForUI::~MessagePumpForUI() {
  g_source_destroy(work_source_);
  g_source_unref(work_source_);
  close(wakeup_pipe_read_);
  close(wakeup_pipe_write_);
}

void MessagePumpForUI::Run(Delegate* delegate) {
#ifndef NDEBUG
  
  
  static PlatformThreadId thread_id = PlatformThread::CurrentId();
  DCHECK(thread_id == PlatformThread::CurrentId()) <<
      "Running MessagePumpForUI on two different threads; "
      "this is unsupported by GLib!";
#endif

  RunState state;
  state.delegate = delegate;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  
  
  
  
  
  state.more_work_is_plausible = true;

  RunState* previous_state = state_;
  state_ = &state;

  
  
  
  while (!state_->should_quit)
    g_main_context_iteration(context_, true);

  state_ = previous_state;
}


int MessagePumpForUI::HandlePrepare() {
  
  
  if (state_->more_work_is_plausible)
    return 0;

  
  
  
  
  
  
  state_->more_work_is_plausible = true;

  
  
  return GetTimeIntervalMilliseconds(delayed_work_time_);
}

void MessagePumpForUI::HandleDispatch() {
  
  
  
  if (wakeup_gpollfd_.revents & G_IO_IN) {
    char msg;
    if (HANDLE_EINTR(read(wakeup_pipe_read_, &msg, 1)) != 1 || msg != '!') {
      NOTREACHED() << "Error reading from the wakeup pipe.";
    }
  }

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
  
  
  
  char msg = '!';
  if (HANDLE_EINTR(write(wakeup_pipe_write_, &msg, 1)) != 1) {
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
  }
}

void MessagePumpForUI::ScheduleDelayedWork(const Time& delayed_work_time) {
  
  
  delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}

}  
