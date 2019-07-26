



#include "base/message_pump_glib.h"

#include <fcntl.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/platform_thread.h"

namespace {


const char kWorkScheduled = '\0';



int GetTimeIntervalMilliseconds(const base::TimeTicks& from) {
  if (from.is_null())
    return -1;

  
  
  
  int delay = static_cast<int>(
      ceil((from - base::TimeTicks::Now()).InMillisecondsF()));

  
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
  
  return static_cast<WorkSource*>(source)->pump->HandleCheck();
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
      context_(g_main_context_default()),
      wakeup_gpollfd_(new GPollFD) {
  
  int fds[2];
  CHECK(pipe(fds) == 0);
  wakeup_pipe_read_  = fds[0];
  wakeup_pipe_write_ = fds[1];
  wakeup_gpollfd_->fd = wakeup_pipe_read_;
  wakeup_gpollfd_->events = G_IO_IN;

  work_source_ = g_source_new(&WorkSourceFuncs, sizeof(WorkSource));
  static_cast<WorkSource*>(work_source_)->pump = this;
  g_source_add_poll(work_source_, wakeup_gpollfd_.get());
  
  g_source_set_priority(work_source_, G_PRIORITY_DEFAULT_IDLE);
  
  g_source_set_can_recurse(work_source_, TRUE);
  g_source_attach(work_source_, context_);
  gdk_event_handler_set(&EventDispatcher, this, NULL);
}

MessagePumpForUI::~MessagePumpForUI() {
  gdk_event_handler_set(reinterpret_cast<GdkEventFunc>(gtk_main_do_event),
                        this, NULL);
  g_source_destroy(work_source_);
  g_source_unref(work_source_);
  close(wakeup_pipe_read_);
  close(wakeup_pipe_write_);
}

void MessagePumpForUI::RunWithDispatcher(Delegate* delegate,
                                         Dispatcher* dispatcher) {
#ifndef NDEBUG
  
  
  static PlatformThreadId thread_id = PlatformThread::CurrentId();
  DCHECK(thread_id == PlatformThread::CurrentId()) <<
      "Running MessagePumpForUI on two different threads; "
      "this is unsupported by GLib!";
#endif

  RunState state;
  state.delegate = delegate;
  state.dispatcher = dispatcher;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  state.has_work = false;

  RunState* previous_state = state_;
  state_ = &state;

  
  
  
  
  
  bool more_work_is_plausible = true;

  
  
  
  for (;;) {
    
    bool block = !more_work_is_plausible;

    
    more_work_is_plausible = g_main_context_iteration(context_, block);
    if (state_->should_quit)
      break;

    more_work_is_plausible |= state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;
  }

  state_ = previous_state;
}


int MessagePumpForUI::HandlePrepare() {
  
  
  if (state_ &&  
      state_->has_work)
    return 0;

  
  
  return GetTimeIntervalMilliseconds(delayed_work_time_);
}

bool MessagePumpForUI::HandleCheck() {
  if (!state_)  
    return false;

  
  
  
  if (wakeup_gpollfd_->revents & G_IO_IN) {
    char msg;
    if (HANDLE_EINTR(read(wakeup_pipe_read_, &msg, 1)) != 1 || msg != '!') {
      NOTREACHED() << "Error reading from the wakeup pipe.";
    }
    
    
    
    state_->has_work = true;
  }

  if (state_->has_work)
    return true;

  if (GetTimeIntervalMilliseconds(delayed_work_time_) == 0) {
    
    
    return true;
  }

  return false;
}

void MessagePumpForUI::HandleDispatch() {
  state_->has_work = false;
  if (state_->delegate->DoWork()) {
    
    
    
    
    state_->has_work = true;
  }

  if (state_->should_quit)
    return;

  state_->delegate->DoDelayedWork(&delayed_work_time_);
}

void MessagePumpForUI::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MessagePumpForUI::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MessagePumpForUI::WillProcessEvent(GdkEvent* event) {
  FOR_EACH_OBSERVER(Observer, observers_, WillProcessEvent(event));
}

void MessagePumpForUI::DidProcessEvent(GdkEvent* event) {
  FOR_EACH_OBSERVER(Observer, observers_, DidProcessEvent(event));
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

void MessagePumpForUI::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  
  
  delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}


void MessagePumpForUI::EventDispatcher(GdkEvent* event, gpointer data) {
  MessagePumpForUI* message_pump = reinterpret_cast<MessagePumpForUI*>(data);

  message_pump->WillProcessEvent(event);
  if (message_pump->state_ &&  
      message_pump->state_->dispatcher) {
    if (!message_pump->state_->dispatcher->Dispatch(event))
      message_pump->state_->should_quit = true;
  } else {
    gtk_main_do_event(event);
  }
  message_pump->DidProcessEvent(event);
}

}  
