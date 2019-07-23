



#ifndef BASE_MESSAGE_LOOP_H_
#define BASE_MESSAGE_LOOP_H_

#include <deque>
#include <queue>
#include <string>
#include <vector>

#include "base/histogram.h"
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































class MessageLoop : public base::MessagePump::Delegate {
 public:
  static void EnableHistogrammer(bool enable_histogrammer);

  
  
  
  
  
  
  
  
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
    TYPE_IO
  };

  
  
  explicit MessageLoop(Type type = TYPE_DEFAULT);
  ~MessageLoop();

  
  Type type() const { return type_; }

  
  void set_thread_name(const std::string& thread_name) {
    DCHECK(thread_name_.empty()) << "Should not rename this thread!";
    thread_name_ = thread_name;
  }
  const std::string& thread_name() const { return thread_name_; }

  
  static MessageLoop* current();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void SetNestableTasksAllowed(bool allowed);
  bool NestableTasksAllowed() const;

  
  
  
  
  void set_exception_restoration(bool restore) {
    exception_restoration_ = restore;
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
    base::Time delayed_run_time;  
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
  virtual bool DoDelayedWork(base::Time* next_delayed_work_time);
  virtual bool DoIdleWork();

  
  
  void StartHistogrammer();

  
  
  
  void HistogramEvent(int event);

  static const LinearHistogram::DescriptionPair event_descriptions_[];
  static bool enable_histogrammer_;

  Type type_;

  
  
  TaskQueue work_queue_;

  
  DelayedTaskQueue delayed_work_queue_;

  
  
  
  TaskQueue deferred_non_nestable_work_queue_;

  scoped_refptr<base::MessagePump> pump_;

  ObserverList<DestructionObserver> destruction_observers_;

  
  
  bool nestable_tasks_allowed_;

  bool exception_restoration_;

  std::string thread_name_;
  
  scoped_ptr<LinearHistogram> message_histogram_;

  
  
  
  
  TaskQueue incoming_queue_;
  
  Lock incoming_queue_lock_;

  RunState* state_;

  
  int next_sequence_num_;

  DISALLOW_COPY_AND_ASSIGN(MessageLoop);
};








class MessageLoopForUI : public MessageLoop {
 public:
  MessageLoopForUI() : MessageLoop(TYPE_UI) {
  }

  
  static MessageLoopForUI* current() {
    MessageLoop* loop = MessageLoop::current();
    DCHECK_EQ(MessageLoop::TYPE_UI, loop->type());
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
#endif  
};




COMPILE_ASSERT(sizeof(MessageLoop) == sizeof(MessageLoopForIO),
               MessageLoopForIO_should_not_have_extra_member_variables);

#endif  
