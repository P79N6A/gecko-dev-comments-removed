



#include "base/message_loop.h"

#include <algorithm>

#include "mozilla/Atomics.h"
#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/message_pump_default.h"
#include "base/string_util.h"
#include "base/thread_local.h"

#if defined(OS_MACOSX)
#include "base/message_pump_mac.h"
#endif
#if defined(OS_POSIX)
#include "base/message_pump_libevent.h"
#endif
#if defined(OS_LINUX) || defined(OS_BSD)
#if defined(MOZ_WIDGET_GTK)
#include "base/message_pump_glib.h"
#endif
#ifdef MOZ_WIDGET_QT
#include "base/message_pump_qt.h"
#endif
#endif
#ifdef ANDROID
#include "base/message_pump_android.h"
#endif
#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#endif

#include "MessagePump.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;



static base::LazyInstance<base::ThreadLocalPointer<MessageLoop> > lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;





static const int kTaskRunEvent = 0x1;
static const int kTimerEvent = 0x2;


static const int kLeastNonZeroMessageId = 1;
static const int kMaxMessageId = 1099;
static const int kNumberOfDistinctMessagesDisplayed = 1100;



#if defined(OS_WIN)



static int SEHFilter(LPTOP_LEVEL_EXCEPTION_FILTER old_filter) {
  ::SetUnhandledExceptionFilter(old_filter);
  return EXCEPTION_CONTINUE_SEARCH;
}



static LPTOP_LEVEL_EXCEPTION_FILTER GetTopSEHFilter() {
  LPTOP_LEVEL_EXCEPTION_FILTER top_filter = NULL;
  top_filter = ::SetUnhandledExceptionFilter(0);
  ::SetUnhandledExceptionFilter(top_filter);
  return top_filter;
}

#endif  




MessageLoop* MessageLoop::current() {
  
  
  
  return lazy_tls_ptr.Pointer()->Get();
}

static mozilla::Atomic<int32_t> message_loop_id_seq(0);

MessageLoop::MessageLoop(Type type)
    : type_(type),
      id_(++message_loop_id_seq),
      nestable_tasks_allowed_(true),
      exception_restoration_(false),
      state_(NULL),
      run_depth_base_(1),
#ifdef OS_WIN
      os_modal_loop_(false),
#endif  
      transient_hang_timeout_(0),
      permanent_hang_timeout_(0),
      next_sequence_num_(0) {
  DCHECK(!current()) << "should only have one message loop per thread";
  lazy_tls_ptr.Pointer()->Set(this);

  switch (type_) {
  case TYPE_MOZILLA_UI:
    pump_ = new mozilla::ipc::MessagePump();
    return;
  case TYPE_MOZILLA_CHILD:
    pump_ = new mozilla::ipc::MessagePumpForChildProcess();
    
    
    
    
    
    run_depth_base_ = 2;
    return;
  case TYPE_MOZILLA_NONMAINTHREAD:
    pump_ = new mozilla::ipc::MessagePumpForNonMainThreads();
    return;
#if defined(OS_WIN)
  case TYPE_MOZILLA_NONMAINUITHREAD:
    pump_ = new mozilla::ipc::MessagePumpForNonMainUIThreads();
    return;
#endif
  }

#if defined(OS_WIN)
  
  if (type_ == TYPE_DEFAULT) {
    pump_ = new base::MessagePumpDefault();
  } else if (type_ == TYPE_IO) {
    pump_ = new base::MessagePumpForIO();
  } else {
    DCHECK(type_ == TYPE_UI);
    pump_ = new base::MessagePumpForUI();
  }
#elif defined(OS_POSIX)
  if (type_ == TYPE_UI) {
#if defined(OS_MACOSX)
    pump_ = base::MessagePumpMac::Create();
#elif defined(OS_LINUX) || defined(OS_BSD)
    pump_ = new base::MessagePumpForUI();
#endif  
  } else if (type_ == TYPE_IO) {
    pump_ = new base::MessagePumpLibevent();
  } else {
    pump_ = new base::MessagePumpDefault();
  }
#endif  
}

MessageLoop::~MessageLoop() {
  DCHECK(this == current());

  
  FOR_EACH_OBSERVER(DestructionObserver, destruction_observers_,
                    WillDestroyCurrentMessageLoop());

  DCHECK(!state_);

  
  
  
  
  
  
  bool did_work;
  for (int i = 0; i < 100; ++i) {
    DeletePendingTasks();
    ReloadWorkQueue();
    
    did_work = DeletePendingTasks();
    if (!did_work)
      break;
  }
  DCHECK(!did_work);

  
  lazy_tls_ptr.Pointer()->Set(NULL);
}

void MessageLoop::AddDestructionObserver(DestructionObserver *obs) {
  DCHECK(this == current());
  destruction_observers_.AddObserver(obs);
}

void MessageLoop::RemoveDestructionObserver(DestructionObserver *obs) {
  DCHECK(this == current());
  destruction_observers_.RemoveObserver(obs);
}

void MessageLoop::Run() {
  AutoRunState save_state(this);
  RunHandler();
}

void MessageLoop::RunAllPending() {
  AutoRunState save_state(this);
  state_->quit_received = true;  
  RunHandler();
}






void MessageLoop::RunHandler() {
#if defined(OS_WIN)
  if (exception_restoration_) {
    LPTOP_LEVEL_EXCEPTION_FILTER current_filter = GetTopSEHFilter();
    MOZ_SEH_TRY {
      RunInternal();
    } MOZ_SEH_EXCEPT(SEHFilter(current_filter)) {
    }
    return;
  }
#endif

  RunInternal();
}



void MessageLoop::RunInternal() {
  DCHECK(this == current());
  pump_->Run(this);
}




bool MessageLoop::ProcessNextDelayedNonNestableTask() {
  if (state_->run_depth > run_depth_base_)
    return false;

  if (deferred_non_nestable_work_queue_.empty())
    return false;

  Task* task = deferred_non_nestable_work_queue_.front().task;
  deferred_non_nestable_work_queue_.pop();

  RunTask(task);
  return true;
}



void MessageLoop::Quit() {
  DCHECK(current() == this);
  if (state_) {
    state_->quit_received = true;
  } else {
    NOTREACHED() << "Must be inside Run to call Quit";
  }
}

void MessageLoop::PostTask(
    const tracked_objects::Location& from_here, Task* task) {
  PostTask_Helper(from_here, task, 0, true);
}

void MessageLoop::PostDelayedTask(
    const tracked_objects::Location& from_here, Task* task, int delay_ms) {
  PostTask_Helper(from_here, task, delay_ms, true);
}

void MessageLoop::PostNonNestableTask(
    const tracked_objects::Location& from_here, Task* task) {
  PostTask_Helper(from_here, task, 0, false);
}

void MessageLoop::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here, Task* task, int delay_ms) {
  PostTask_Helper(from_here, task, delay_ms, false);
}

void MessageLoop::PostIdleTask(
    const tracked_objects::Location& from_here, Task* task) {
  DCHECK(current() == this);

#ifdef MOZ_TASK_TRACER
  task = mozilla::tasktracer::CreateTracedTask(task);
#endif

  task->SetBirthPlace(from_here);
  PendingTask pending_task(task, false);
  deferred_non_nestable_work_queue_.push(pending_task);
}


void MessageLoop::PostTask_Helper(
    const tracked_objects::Location& from_here, Task* task, int delay_ms,
    bool nestable) {

#ifdef MOZ_TASK_TRACER
  task = mozilla::tasktracer::CreateTracedTask(task);
#endif

  task->SetBirthPlace(from_here);

  PendingTask pending_task(task, nestable);

  if (delay_ms > 0) {
    pending_task.delayed_run_time =
        TimeTicks::Now() + TimeDelta::FromMilliseconds(delay_ms);
  } else {
    DCHECK(delay_ms == 0) << "delay should not be negative";
  }

  
  
  

  nsRefPtr<base::MessagePump> pump;
  {
    AutoLock locked(incoming_queue_lock_);
    incoming_queue_.push(pending_task);
    pump = pump_;
  }
  
  
  
  

  pump->ScheduleWork();
}

void MessageLoop::SetNestableTasksAllowed(bool allowed) {
  if (nestable_tasks_allowed_ != allowed) {
    nestable_tasks_allowed_ = allowed;
    if (!nestable_tasks_allowed_)
      return;
    
    pump_->ScheduleWorkForNestedLoop();
  }
}

void MessageLoop::ScheduleWork() {
  
  pump_->ScheduleWork();
}

bool MessageLoop::NestableTasksAllowed() const {
  return nestable_tasks_allowed_;
}



void MessageLoop::RunTask(Task* task) {
  DCHECK(nestable_tasks_allowed_);
  
  nestable_tasks_allowed_ = false;

  task->Run();
  delete task;

  nestable_tasks_allowed_ = true;
}

bool MessageLoop::DeferOrRunPendingTask(const PendingTask& pending_task) {
  if (pending_task.nestable || state_->run_depth <= run_depth_base_) {
    RunTask(pending_task.task);
    
    
    return true;
  }

  
  
  deferred_non_nestable_work_queue_.push(pending_task);
  return false;
}

void MessageLoop::AddToDelayedWorkQueue(const PendingTask& pending_task) {
  
  
  
  
  PendingTask new_pending_task(pending_task);
  new_pending_task.sequence_num = next_sequence_num_++;
  delayed_work_queue_.push(new_pending_task);
}

void MessageLoop::ReloadWorkQueue() {
  
  
  
  
  if (!work_queue_.empty())
    return;  

  
  {
    AutoLock lock(incoming_queue_lock_);
    if (incoming_queue_.empty())
      return;
    std::swap(incoming_queue_, work_queue_);
    DCHECK(incoming_queue_.empty());
  }
}

bool MessageLoop::DeletePendingTasks() {
  MOZ_ASSERT(work_queue_.empty());
  bool did_work = !deferred_non_nestable_work_queue_.empty();
  while (!deferred_non_nestable_work_queue_.empty()) {
    Task* task = deferred_non_nestable_work_queue_.front().task;
    deferred_non_nestable_work_queue_.pop();
    delete task;
  }
  did_work |= !delayed_work_queue_.empty();
  while (!delayed_work_queue_.empty()) {
    Task* task = delayed_work_queue_.top().task;
    delayed_work_queue_.pop();
    delete task;
  }
  return did_work;
}

bool MessageLoop::DoWork() {
  if (!nestable_tasks_allowed_) {
    
    return false;
  }

  for (;;) {
    ReloadWorkQueue();
    if (work_queue_.empty())
      break;

    
    do {
      PendingTask pending_task = work_queue_.front();
      work_queue_.pop();
      if (!pending_task.delayed_run_time.is_null()) {
        AddToDelayedWorkQueue(pending_task);
        
        if (delayed_work_queue_.top().task == pending_task.task)
          pump_->ScheduleDelayedWork(pending_task.delayed_run_time);
      } else {
        if (DeferOrRunPendingTask(pending_task))
          return true;
      }
    } while (!work_queue_.empty());
  }

  
  return false;
}

bool MessageLoop::DoDelayedWork(TimeTicks* next_delayed_work_time) {
  if (!nestable_tasks_allowed_ || delayed_work_queue_.empty()) {
    *next_delayed_work_time = TimeTicks();
    return false;
  }

  if (delayed_work_queue_.top().delayed_run_time > TimeTicks::Now()) {
    *next_delayed_work_time = delayed_work_queue_.top().delayed_run_time;
    return false;
  }

  PendingTask pending_task = delayed_work_queue_.top();
  delayed_work_queue_.pop();

  if (!delayed_work_queue_.empty())
    *next_delayed_work_time = delayed_work_queue_.top().delayed_run_time;

  return DeferOrRunPendingTask(pending_task);
}

bool MessageLoop::DoIdleWork() {
  if (ProcessNextDelayedNonNestableTask())
    return true;

  if (state_->quit_received)
    pump_->Quit();

  return false;
}




MessageLoop::AutoRunState::AutoRunState(MessageLoop* loop) : loop_(loop) {
  
  previous_state_ = loop_->state_;
  if (previous_state_) {
    run_depth = previous_state_->run_depth + 1;
  } else {
    run_depth = 1;
  }
  loop_->state_ = this;

  
  quit_received = false;
#if defined(OS_WIN)
  dispatcher = NULL;
#endif
}

MessageLoop::AutoRunState::~AutoRunState() {
  loop_->state_ = previous_state_;
}




bool MessageLoop::PendingTask::operator<(const PendingTask& other) const {
  
  
  

  if (delayed_run_time < other.delayed_run_time)
    return false;

  if (delayed_run_time > other.delayed_run_time)
    return true;

  
  
  return (sequence_num - other.sequence_num) > 0;
}




#if defined(OS_WIN)

void MessageLoopForUI::Run(Dispatcher* dispatcher) {
  AutoRunState save_state(this);
  state_->dispatcher = dispatcher;
  RunHandler();
}

void MessageLoopForUI::AddObserver(Observer* observer) {
  pump_win()->AddObserver(observer);
}

void MessageLoopForUI::RemoveObserver(Observer* observer) {
  pump_win()->RemoveObserver(observer);
}

void MessageLoopForUI::WillProcessMessage(const MSG& message) {
  pump_win()->WillProcessMessage(message);
}
void MessageLoopForUI::DidProcessMessage(const MSG& message) {
  pump_win()->DidProcessMessage(message);
}
void MessageLoopForUI::PumpOutPendingPaintMessages() {
  pump_ui()->PumpOutPendingPaintMessages();
}

#endif  




#if defined(OS_WIN)

void MessageLoopForIO::RegisterIOHandler(HANDLE file, IOHandler* handler) {
  pump_io()->RegisterIOHandler(file, handler);
}

bool MessageLoopForIO::WaitForIOCompletion(DWORD timeout, IOHandler* filter) {
  return pump_io()->WaitForIOCompletion(timeout, filter);
}

#elif defined(OS_POSIX)

bool MessageLoopForIO::WatchFileDescriptor(int fd,
                                           bool persistent,
                                           Mode mode,
                                           FileDescriptorWatcher *controller,
                                           Watcher *delegate) {
  return pump_libevent()->WatchFileDescriptor(
      fd,
      persistent,
      static_cast<base::MessagePumpLibevent::Mode>(mode),
      controller,
      delegate);
}

bool
MessageLoopForIO::CatchSignal(int sig,
                              SignalEvent* sigevent,
                              SignalWatcher* delegate)
{
  return pump_libevent()->CatchSignal(sig, sigevent, delegate);
}

#endif
