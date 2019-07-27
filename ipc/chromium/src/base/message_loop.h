



#ifndef BASE_MESSAGE_LOOP_H_
#define BASE_MESSAGE_LOOP_H_

#include <deque>
#include <queue>
#include <string>
#include <vector>

#include <map>
#include "base/lock.h"
#include "base/message_pump.h"
#include "base/observer_list.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "base/timer.h"

#if defined(OS_WIN)


#include "base/message_pump_win.h"
#elif defined(OS_POSIX)
#include "base/message_pump_libevent.h"
#endif

namespace mozilla {
namespace ipc {

class DoWorkRunnable;

} 
} 































class MessageLoop : public base::MessagePump::Delegate {

  friend class mozilla::ipc::DoWorkRunnable;

public:
  
  
  
  
  
  
  
  
  class DestructionObserver {
   public:
    virtual ~DestructionObserver() {}
    virtual void WillDestroyCurrentMessageLoop() = 0;
  };

  
  
  void AddDestructionObserver(DestructionObserver* destruction_observer);

  
  
  void RemoveDestructionObserver(DestructionObserver* destruction_observer);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  void PostTask(
      const tracked_objects::Location& from_here, Task* task);

  void PostDelayedTask(
      const tracked_objects::Location& from_here, Task* task, int delay_ms);

  void PostNonNestableTask(
      const tracked_objects::Location& from_here, Task* task);

  void PostNonNestableDelayedTask(
      const tracked_objects::Location& from_here, Task* task, int delay_ms);

  
  void PostIdleTask(
      const tracked_objects::Location& from_here, Task* task);

  
  
  
  
  
  
  
  
  
  template <class T>
  void DeleteSoon(const tracked_objects::Location& from_here, T* object) {
    PostNonNestableTask(from_here, new DeleteTask<T>(object));
  }

  
  
  
  
  
  
  
  
  
  
  template <class T>
  void ReleaseSoon(const tracked_objects::Location& from_here, T* object) {
    PostNonNestableTask(from_here, new ReleaseTask<T>(object));
  }

  
  void Run();

  
  
  void RunAllPending();

  
  
  
  
  
  
  
  
  
  void Quit();

  
  
  class QuitTask : public Task {
   public:
    virtual void Run() {
      MessageLoop::current()->Quit();
    }
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum Type {
    TYPE_DEFAULT,
    TYPE_UI,
    TYPE_IO,
    TYPE_MOZILLA_CHILD,
    TYPE_MOZILLA_UI,
    TYPE_MOZILLA_NONMAINTHREAD,
    TYPE_MOZILLA_NONMAINUITHREAD
  };

  
  
  explicit MessageLoop(Type type = TYPE_DEFAULT);
  ~MessageLoop();

  
  Type type() const { return type_; }

  
  int32_t id() const { return id_; }

  
  void set_thread_name(const std::string& thread_name) {
    DCHECK(thread_name_.empty()) << "Should not rename this thread!";
    thread_name_ = thread_name;
  }
  const std::string& thread_name() const { return thread_name_; }

  
  static MessageLoop* current();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void SetNestableTasksAllowed(bool allowed);
  void ScheduleWork();
  bool NestableTasksAllowed() const;

  
  
  
  
  void set_exception_restoration(bool restore) {
    exception_restoration_ = restore;
  }

#if defined(OS_WIN)
  void set_os_modal_loop(bool os_modal_loop) {
    os_modal_loop_ = os_modal_loop;
  }

  bool & os_modal_loop() {
    return os_modal_loop_;
  }
#endif  

  
  
  void set_hang_timeouts(uint32_t transient_timeout_ms,
                         uint32_t permanent_timeout_ms) {
    transient_hang_timeout_ = transient_timeout_ms;
    permanent_hang_timeout_ = permanent_timeout_ms;
  }
  uint32_t transient_hang_timeout() const {
    return transient_hang_timeout_;
  }
  uint32_t permanent_hang_timeout() const {
    return permanent_hang_timeout_;
  }

  
 protected:
  struct RunState {
    
    int run_depth;

    
    
    bool quit_received;

#if defined(OS_WIN)
    base::MessagePumpWin::Dispatcher* dispatcher;
#endif
  };

  class AutoRunState : RunState {
   public:
    explicit AutoRunState(MessageLoop* loop);
    ~AutoRunState();
   private:
    MessageLoop* loop_;
    RunState* previous_state_;
  };

  
  struct PendingTask {
    Task* task;                        
    base::TimeTicks delayed_run_time;  
    int sequence_num;                  
    bool nestable;                     

    PendingTask(Task* task, bool nestable)
        : task(task), sequence_num(0), nestable(nestable) {
    }

    
    bool operator<(const PendingTask& other) const;
  };

  typedef std::queue<PendingTask> TaskQueue;
  typedef std::priority_queue<PendingTask> DelayedTaskQueue;

#if defined(OS_WIN)
  base::MessagePumpWin* pump_win() {
    return static_cast<base::MessagePumpWin*>(pump_.get());
  }
#elif defined(OS_POSIX)
  base::MessagePumpLibevent* pump_libevent() {
    return static_cast<base::MessagePumpLibevent*>(pump_.get());
  }
#endif

  
  
  
  
  void RunHandler();

  
  
  
  void RunInternal();

  
  bool ProcessNextDelayedNonNestableTask();

  
  
  
  
  
  
  
  
  
  
  bool QueueOrRunTask(Task* new_task);

  
  void RunTask(Task* task);

  
  
  bool DeferOrRunPendingTask(const PendingTask& pending_task);

  
  void AddToDelayedWorkQueue(const PendingTask& pending_task);

  
  
  
  void ReloadWorkQueue();

  
  
  
  bool DeletePendingTasks();

  
  void PostTask_Helper(const tracked_objects::Location& from_here, Task* task,
                       int delay_ms, bool nestable);

  
  virtual bool DoWork();
  virtual bool DoDelayedWork(base::TimeTicks* next_delayed_work_time);
  virtual bool DoIdleWork();

  Type type_;
  int32_t id_;

  
  
  TaskQueue work_queue_;

  
  DelayedTaskQueue delayed_work_queue_;

  
  
  
  TaskQueue deferred_non_nestable_work_queue_;

  scoped_refptr<base::MessagePump> pump_;

  base::ObserverList<DestructionObserver> destruction_observers_;

  
  
  bool nestable_tasks_allowed_;

  bool exception_restoration_;

  std::string thread_name_;

  
  
  
  
  TaskQueue incoming_queue_;
  
  Lock incoming_queue_lock_;

  RunState* state_;
  int run_depth_base_;

#if defined(OS_WIN)
  
  
  bool os_modal_loop_;
#endif

  
  uint32_t transient_hang_timeout_;
  uint32_t permanent_hang_timeout_;

  
  int next_sequence_num_;

  DISALLOW_COPY_AND_ASSIGN(MessageLoop);
};








class MessageLoopForUI : public MessageLoop {
 public:
  explicit MessageLoopForUI(Type type=TYPE_UI) : MessageLoop(type) {
  }

  
  static MessageLoopForUI* current() {
    MessageLoop* loop = MessageLoop::current();
    if (!loop)
      return NULL;
    Type type = loop->type();
    DCHECK(type == MessageLoop::TYPE_UI ||
           type == MessageLoop::TYPE_MOZILLA_UI ||
           type == MessageLoop::TYPE_MOZILLA_CHILD);
    return static_cast<MessageLoopForUI*>(loop);
  }

#if defined(OS_WIN)
  typedef base::MessagePumpWin::Dispatcher Dispatcher;
  typedef base::MessagePumpWin::Observer Observer;

  
  void Run(Dispatcher* dispatcher);
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void WillProcessMessage(const MSG& message);
  void DidProcessMessage(const MSG& message);
  void PumpOutPendingPaintMessages();

 protected:
  
  base::MessagePumpForUI* pump_ui() {
    return static_cast<base::MessagePumpForUI*>(pump_.get());
  }
#endif  
};




COMPILE_ASSERT(sizeof(MessageLoop) == sizeof(MessageLoopForUI),
               MessageLoopForUI_should_not_have_extra_member_variables);








class MessageLoopForIO : public MessageLoop {
 public:
  MessageLoopForIO() : MessageLoop(TYPE_IO) {
  }

  
  static MessageLoopForIO* current() {
    MessageLoop* loop = MessageLoop::current();
    DCHECK_EQ(MessageLoop::TYPE_IO, loop->type());
    return static_cast<MessageLoopForIO*>(loop);
  }

#if defined(OS_WIN)
  typedef base::MessagePumpForIO::IOHandler IOHandler;
  typedef base::MessagePumpForIO::IOContext IOContext;

  
  void RegisterIOHandler(HANDLE file_handle, IOHandler* handler);
  bool WaitForIOCompletion(DWORD timeout, IOHandler* filter);

 protected:
  
  base::MessagePumpForIO* pump_io() {
    return static_cast<base::MessagePumpForIO*>(pump_.get());
  }

#elif defined(OS_POSIX)
  typedef base::MessagePumpLibevent::Watcher Watcher;
  typedef base::MessagePumpLibevent::FileDescriptorWatcher
      FileDescriptorWatcher;
  typedef base::LineWatcher LineWatcher;

  enum Mode {
    WATCH_READ = base::MessagePumpLibevent::WATCH_READ,
    WATCH_WRITE = base::MessagePumpLibevent::WATCH_WRITE,
    WATCH_READ_WRITE = base::MessagePumpLibevent::WATCH_READ_WRITE
  };

  
  bool WatchFileDescriptor(int fd,
                           bool persistent,
                           Mode mode,
                           FileDescriptorWatcher *controller,
                           Watcher *delegate);

  typedef base::MessagePumpLibevent::SignalEvent SignalEvent;
  typedef base::MessagePumpLibevent::SignalWatcher SignalWatcher;
  bool CatchSignal(int sig,
                   SignalEvent* sigevent,
                   SignalWatcher* delegate);

#endif  
};




COMPILE_ASSERT(sizeof(MessageLoop) == sizeof(MessageLoopForIO),
               MessageLoopForIO_should_not_have_extra_member_variables);

#endif  
