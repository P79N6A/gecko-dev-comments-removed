



#include "base/logging.h"
#include "base/message_loop.h"
#include "base/platform_thread.h"
#include "base/ref_counted.h"
#include "base/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include "base/message_pump_win.h"
#include "base/scoped_handle.h"
#endif
#if defined(OS_POSIX)
#include "base/message_pump_libevent.h"
#endif

using base::Thread;
using base::Time;
using base::TimeDelta;




namespace {

class MessageLoopTest : public testing::Test {};

class Foo : public base::RefCounted<Foo> {
 public:
  Foo() : test_count_(0) {
  }

  void Test0() {
    ++test_count_;
  }

  void Test1ConstRef(const std::string& a) {
    ++test_count_;
    result_.append(a);
  }

  void Test1Ptr(std::string* a) {
    ++test_count_;
    result_.append(*a);
  }

  void Test1Int(int a) {
    test_count_ += a;
  }

  void Test2Ptr(std::string* a, std::string* b) {
    ++test_count_;
    result_.append(*a);
    result_.append(*b);
  }

  void Test2Mixed(const std::string& a, std::string* b) {
    ++test_count_;
    result_.append(a);
    result_.append(*b);
  }

  int test_count() const { return test_count_; }
  const std::string& result() const { return result_; }

 private:
  int test_count_;
  std::string result_;
};

class QuitMsgLoop : public base::RefCounted<QuitMsgLoop> {
 public:
  void QuitNow() {
    MessageLoop::current()->Quit();
  }
};

void RunTest_PostTask(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  scoped_refptr<Foo> foo = new Foo();
  std::string a("a"), b("b"), c("c"), d("d");
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test0));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
    foo.get(), &Foo::Test1ConstRef, a));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test1Ptr, &b));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test1Int, 100));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test2Ptr, &a, &c));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
    foo.get(), &Foo::Test2Mixed, a, &d));

  
  scoped_refptr<QuitMsgLoop> quit = new QuitMsgLoop();
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      quit.get(), &QuitMsgLoop::QuitNow));

  
  MessageLoop::current()->Run();

  EXPECT_EQ(foo->test_count(), 105);
  EXPECT_EQ(foo->result(), "abacad");
}

void RunTest_PostTask_SEH(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  scoped_refptr<Foo> foo = new Foo();
  std::string a("a"), b("b"), c("c"), d("d");
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test0));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test1ConstRef, a));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test1Ptr, &b));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test1Int, 100));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test2Ptr, &a, &c));
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      foo.get(), &Foo::Test2Mixed, a, &d));

  
  scoped_refptr<QuitMsgLoop> quit = new QuitMsgLoop();
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      quit.get(), &QuitMsgLoop::QuitNow));

  
  MessageLoop::current()->set_exception_restoration(true);
  MessageLoop::current()->Run();
  MessageLoop::current()->set_exception_restoration(false);

  EXPECT_EQ(foo->test_count(), 105);
  EXPECT_EQ(foo->result(), "abacad");
}


class SlowTask : public Task {
 public:
  SlowTask(int pause_ms, int* quit_counter)
      : pause_ms_(pause_ms), quit_counter_(quit_counter) {
  }
  virtual void Run() {
    PlatformThread::Sleep(pause_ms_);
    if (--(*quit_counter_) == 0)
      MessageLoop::current()->Quit();
  }
 private:
  int pause_ms_;
  int* quit_counter_;
};



class RecordRunTimeTask : public SlowTask {
 public:
  RecordRunTimeTask(Time* run_time, int* quit_counter)
      : SlowTask(10, quit_counter), run_time_(run_time) {
  }
  virtual void Run() {
    *run_time_ = Time::Now();
    
    
    
    SlowTask::Run();
  }
 private:
  Time* run_time_;
};

void RunTest_PostDelayedTask_Basic(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  

  const int kDelayMS = 100;

  int num_tasks = 1;
  Time run_time;

  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time, &num_tasks), kDelayMS);

  Time time_before_run = Time::Now();
  loop.Run();
  Time time_after_run = Time::Now();

  EXPECT_EQ(0, num_tasks);
  EXPECT_LT(kDelayMS, (time_after_run - time_before_run).InMilliseconds());
}

void RunTest_PostDelayedTask_InDelayOrder(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  

  int num_tasks = 2;
  Time run_time1, run_time2;

  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time1, &num_tasks), 200);
  
  
  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time2, &num_tasks), 10);

  loop.Run();
  EXPECT_EQ(0, num_tasks);

  EXPECT_TRUE(run_time2 < run_time1);
}

void RunTest_PostDelayedTask_InPostOrder(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  
  
  
  
  
  

  const int kDelayMS = 100;

  int num_tasks = 2;
  Time run_time1, run_time2;

  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time1, &num_tasks), kDelayMS);
  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time2, &num_tasks), kDelayMS);

  loop.Run();
  EXPECT_EQ(0, num_tasks);

  EXPECT_TRUE(run_time1 < run_time2);
}

void RunTest_PostDelayedTask_InPostOrder_2(
    MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  

  const int kPauseMS = 50;

  int num_tasks = 2;
  Time run_time;

  loop.PostTask(
      FROM_HERE, new SlowTask(kPauseMS, &num_tasks));
  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time, &num_tasks), 10);

  Time time_before_run = Time::Now();
  loop.Run();
  Time time_after_run = Time::Now();

  EXPECT_EQ(0, num_tasks);

  EXPECT_LT(kPauseMS, (time_after_run - time_before_run).InMilliseconds());
}

void RunTest_PostDelayedTask_InPostOrder_3(
    MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  
  
  
  

  int num_tasks = 11;
  Time run_time1, run_time2;

  
  for (int i = 1; i < num_tasks; ++i)
    loop.PostTask(FROM_HERE, new RecordRunTimeTask(&run_time1, &num_tasks));

  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time2, &num_tasks), 1);

  loop.Run();
  EXPECT_EQ(0, num_tasks);

  EXPECT_TRUE(run_time2 > run_time1);
}

void RunTest_PostDelayedTask_SharedTimer(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  
  

  
  
  int num_tasks = 1;
  Time run_time1, run_time2;

  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time1, &num_tasks), 1000000);
  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time2, &num_tasks), 10);

  Time start_time = Time::Now();

  loop.Run();
  EXPECT_EQ(0, num_tasks);

  
  TimeDelta total_time = Time::Now() - start_time;
  EXPECT_GT(5000, total_time.InMilliseconds());

  
  
  
  PlatformThread::Sleep(100);
  loop.RunAllPending();

  EXPECT_TRUE(run_time1.is_null());
  EXPECT_FALSE(run_time2.is_null());
}

#if defined(OS_WIN)

class SubPumpTask : public Task {
 public:
  virtual void Run() {
    MessageLoop::current()->SetNestableTasksAllowed(true);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    MessageLoop::current()->Quit();
  }
};

class SubPumpQuitTask : public Task {
 public:
  SubPumpQuitTask() {
  }
  virtual void Run() {
    PostQuitMessage(0);
  }
};

void RunTest_PostDelayedTask_SharedTimer_SubPump() {
  MessageLoop loop(MessageLoop::TYPE_UI);

  
  

  
  
  int num_tasks = 1;
  Time run_time;

  loop.PostTask(FROM_HERE, new SubPumpTask());

  
  loop.PostDelayedTask(
      FROM_HERE, new RecordRunTimeTask(&run_time, &num_tasks), 1000000);

  
  loop.PostDelayedTask(
      FROM_HERE, new SubPumpQuitTask(), 10);

  Time start_time = Time::Now();

  loop.Run();
  EXPECT_EQ(1, num_tasks);

  
  TimeDelta total_time = Time::Now() - start_time;
  EXPECT_GT(5000, total_time.InMilliseconds());

  
  
  
  PlatformThread::Sleep(100);
  loop.RunAllPending();

  EXPECT_TRUE(run_time.is_null());
}

#endif  

class RecordDeletionTask : public Task {
 public:
  RecordDeletionTask(Task* post_on_delete, bool* was_deleted)
      : post_on_delete_(post_on_delete), was_deleted_(was_deleted) {
  }
  ~RecordDeletionTask() {
    *was_deleted_ = true;
    if (post_on_delete_)
      MessageLoop::current()->PostTask(FROM_HERE, post_on_delete_);
  }
  virtual void Run() {}
 private:
  Task* post_on_delete_;
  bool* was_deleted_;
};

void RunTest_EnsureTaskDeletion(MessageLoop::Type message_loop_type) {
  bool a_was_deleted = false;
  bool b_was_deleted = false;
  {
    MessageLoop loop(message_loop_type);
    loop.PostTask(
        FROM_HERE, new RecordDeletionTask(NULL, &a_was_deleted));
    loop.PostDelayedTask(
        FROM_HERE, new RecordDeletionTask(NULL, &b_was_deleted), 1000);
  }
  EXPECT_TRUE(a_was_deleted);
  EXPECT_TRUE(b_was_deleted);
}

void RunTest_EnsureTaskDeletion_Chain(MessageLoop::Type message_loop_type) {
  bool a_was_deleted = false;
  bool b_was_deleted = false;
  bool c_was_deleted = false;
  {
    MessageLoop loop(message_loop_type);
    RecordDeletionTask* a = new RecordDeletionTask(NULL, &a_was_deleted);
    RecordDeletionTask* b = new RecordDeletionTask(a, &b_was_deleted);
    RecordDeletionTask* c = new RecordDeletionTask(b, &c_was_deleted);
    loop.PostTask(FROM_HERE, c);
  }
  EXPECT_TRUE(a_was_deleted);
  EXPECT_TRUE(b_was_deleted);
  EXPECT_TRUE(c_was_deleted);
}

class NestingTest : public Task {
 public:
  explicit NestingTest(int* depth) : depth_(depth) {
  }
  void Run() {
    if (*depth_ > 0) {
      *depth_ -= 1;
      MessageLoop::current()->PostTask(FROM_HERE, new NestingTest(depth_));

      MessageLoop::current()->SetNestableTasksAllowed(true);
      MessageLoop::current()->Run();
    }
    MessageLoop::current()->Quit();
  }
 private:
  int* depth_;
};

#if defined(OS_WIN)

LONG WINAPI BadExceptionHandler(EXCEPTION_POINTERS *ex_info) {
  ADD_FAILURE() << "bad exception handler";
  ::ExitProcess(ex_info->ExceptionRecord->ExceptionCode);
  return EXCEPTION_EXECUTE_HANDLER;
}



class CrasherTask : public Task {
 public:
  
  
  explicit CrasherTask(bool trash_SEH_handler)
      : trash_SEH_handler_(trash_SEH_handler) {
  }
  void Run() {
    PlatformThread::Sleep(1);
    if (trash_SEH_handler_)
      ::SetUnhandledExceptionFilter(&BadExceptionHandler);
    
    

#if defined(_M_IX86)

    __asm {
      mov eax, dword ptr [CrasherTask::bad_array_]
      mov byte ptr [eax], 66
    }

#elif defined(_M_X64)

    bad_array_[0] = 66;

#else
#error "needs architecture support"
#endif

    MessageLoop::current()->Quit();
  }
  
  static void FixError() {
    bad_array_ = &valid_store_;
  }

 private:
  bool trash_SEH_handler_;
  static volatile char* bad_array_;
  static char valid_store_;
};

volatile char* CrasherTask::bad_array_ = 0;
char CrasherTask::valid_store_ = 0;




LONG WINAPI HandleCrasherTaskException(EXCEPTION_POINTERS *ex_info) {
  if (ex_info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
    return EXCEPTION_EXECUTE_HANDLER;

  CrasherTask::FixError();

#if defined(_M_IX86)

  ex_info->ContextRecord->Eip -= 5;

#elif defined(_M_X64)

  ex_info->ContextRecord->Rip -= 5;

#endif

  return EXCEPTION_CONTINUE_EXECUTION;
}

void RunTest_Crasher(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  if (::IsDebuggerPresent())
    return;

  LPTOP_LEVEL_EXCEPTION_FILTER old_SEH_filter =
      ::SetUnhandledExceptionFilter(&HandleCrasherTaskException);

  MessageLoop::current()->PostTask(FROM_HERE, new CrasherTask(false));
  MessageLoop::current()->set_exception_restoration(true);
  MessageLoop::current()->Run();
  MessageLoop::current()->set_exception_restoration(false);

  ::SetUnhandledExceptionFilter(old_SEH_filter);
}

void RunTest_CrasherNasty(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  if (::IsDebuggerPresent())
    return;

  LPTOP_LEVEL_EXCEPTION_FILTER old_SEH_filter =
      ::SetUnhandledExceptionFilter(&HandleCrasherTaskException);

  MessageLoop::current()->PostTask(FROM_HERE, new CrasherTask(true));
  MessageLoop::current()->set_exception_restoration(true);
  MessageLoop::current()->Run();
  MessageLoop::current()->set_exception_restoration(false);

  ::SetUnhandledExceptionFilter(old_SEH_filter);
}

#endif  

void RunTest_Nesting(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  int depth = 100;
  MessageLoop::current()->PostTask(FROM_HERE, new NestingTest(&depth));
  MessageLoop::current()->Run();
  EXPECT_EQ(depth, 0);
}

const wchar_t* const kMessageBoxTitle = L"MessageLoop Unit Test";

enum TaskType {
  MESSAGEBOX,
  ENDDIALOG,
  RECURSIVE,
  TIMEDMESSAGELOOP,
  QUITMESSAGELOOP,
  ORDERERD,
  PUMPS,
};


struct TaskItem {
  TaskItem(TaskType t, int c, bool s)
      : type(t),
        cookie(c),
        start(s) {
  }

  TaskType type;
  int cookie;
  bool start;

  bool operator == (const TaskItem& other) const {
    return type == other.type && cookie == other.cookie && start == other.start;
  }
};

typedef std::vector<TaskItem> TaskList;

std::ostream& operator <<(std::ostream& os, TaskType type) {
  switch (type) {
  case MESSAGEBOX:        os << "MESSAGEBOX"; break;
  case ENDDIALOG:         os << "ENDDIALOG"; break;
  case RECURSIVE:         os << "RECURSIVE"; break;
  case TIMEDMESSAGELOOP:  os << "TIMEDMESSAGELOOP"; break;
  case QUITMESSAGELOOP:   os << "QUITMESSAGELOOP"; break;
  case ORDERERD:          os << "ORDERERD"; break;
  case PUMPS:             os << "PUMPS"; break;
  default:
    NOTREACHED();
    os << "Unknown TaskType";
    break;
  }
  return os;
}

std::ostream& operator <<(std::ostream& os, const TaskItem& item) {
  if (item.start)
    return os << item.type << " " << item.cookie << " starts";
  else
    return os << item.type << " " << item.cookie << " ends";
}


class OrderedTasks : public Task {
 public:
  OrderedTasks(TaskList* order, int cookie)
      : order_(order),
        type_(ORDERERD),
        cookie_(cookie) {
  }
  OrderedTasks(TaskList* order, TaskType type, int cookie)
      : order_(order),
        type_(type),
        cookie_(cookie) {
  }

  void RunStart() {
    TaskItem item(type_, cookie_, true);
    DLOG(INFO) << item;
    order_->push_back(item);
  }
  void RunEnd() {
    TaskItem item(type_, cookie_, false);
    DLOG(INFO) << item;
    order_->push_back(item);
  }

  virtual void Run() {
    RunStart();
    RunEnd();
  }

 protected:
  TaskList* order() const {
    return order_;
  }

  int cookie() const {
    return cookie_;
  }

 private:
  TaskList* order_;
  TaskType type_;
  int cookie_;
};

#if defined(OS_WIN)




class MessageBoxTask : public OrderedTasks {
 public:
  MessageBoxTask(TaskList* order, int cookie, bool is_reentrant)
      : OrderedTasks(order, MESSAGEBOX, cookie),
        is_reentrant_(is_reentrant) {
  }

  virtual void Run() {
    RunStart();
    if (is_reentrant_)
      MessageLoop::current()->SetNestableTasksAllowed(true);
    MessageBox(NULL, L"Please wait...", kMessageBoxTitle, MB_OK);
    RunEnd();
  }

 private:
  bool is_reentrant_;
};


class EndDialogTask : public OrderedTasks {
 public:
  EndDialogTask(TaskList* order, int cookie)
      : OrderedTasks(order, ENDDIALOG, cookie) {
  }

  virtual void Run() {
    RunStart();
    HWND window = GetActiveWindow();
    if (window != NULL) {
      EXPECT_NE(EndDialog(window, IDCONTINUE), 0);
      
      
      RunEnd();
    }
  }
};

#endif  

class RecursiveTask : public OrderedTasks {
 public:
  RecursiveTask(int depth, TaskList* order, int cookie, bool is_reentrant)
      : OrderedTasks(order, RECURSIVE, cookie),
        depth_(depth),
        is_reentrant_(is_reentrant) {
  }

  virtual void Run() {
    RunStart();
    if (depth_ > 0) {
      if (is_reentrant_)
        MessageLoop::current()->SetNestableTasksAllowed(true);
      MessageLoop::current()->PostTask(FROM_HERE,
          new RecursiveTask(depth_ - 1, order(), cookie(), is_reentrant_));
    }
    RunEnd();
  }

 private:
  int depth_;
  bool is_reentrant_;
};

class QuitTask : public OrderedTasks {
 public:
  QuitTask(TaskList* order, int cookie)
      : OrderedTasks(order, QUITMESSAGELOOP, cookie) {
  }

  virtual void Run() {
    RunStart();
    MessageLoop::current()->Quit();
    RunEnd();
  }
};

#if defined(OS_WIN)

class Recursive2Tasks : public Task {
 public:
  Recursive2Tasks(MessageLoop* target,
                  HANDLE event,
                  bool expect_window,
                  TaskList* order,
                  bool is_reentrant)
      : target_(target),
        event_(event),
        expect_window_(expect_window),
        order_(order),
        is_reentrant_(is_reentrant) {
  }

  virtual void Run() {
    target_->PostTask(FROM_HERE,
                      new RecursiveTask(2, order_, 1, is_reentrant_));
    target_->PostTask(FROM_HERE,
                      new MessageBoxTask(order_, 2, is_reentrant_));
    target_->PostTask(FROM_HERE,
                      new RecursiveTask(2, order_, 3, is_reentrant_));
    
    
    
    
    
    
    target_->PostTask(FROM_HERE, new EndDialogTask(order_, 4));
    target_->PostTask(FROM_HERE, new QuitTask(order_, 5));

    
    
    ASSERT_TRUE(SetEvent(event_));

    
    
    for (; expect_window_;) {
      HWND window = FindWindow(L"#32770", kMessageBoxTitle);
      if (window) {
        
        for (;;) {
          HWND button = FindWindowEx(window, NULL, L"Button", NULL);
          if (button != NULL) {
            EXPECT_TRUE(0 == SendMessage(button, WM_LBUTTONDOWN, 0, 0));
            EXPECT_TRUE(0 == SendMessage(button, WM_LBUTTONUP, 0, 0));
            break;
          }
        }
        break;
      }
    }
  }

 private:
  MessageLoop* target_;
  HANDLE event_;
  TaskList* order_;
  bool expect_window_;
  bool is_reentrant_;
};

#endif  

void RunTest_RecursiveDenial1(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  EXPECT_TRUE(MessageLoop::current()->NestableTasksAllowed());
  TaskList order;
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new RecursiveTask(2, &order, 1, false));
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new RecursiveTask(2, &order, 2, false));
  MessageLoop::current()->PostTask(FROM_HERE, new QuitTask(&order, 3));

  MessageLoop::current()->Run();

  
  ASSERT_EQ(14U, order.size());
  EXPECT_EQ(order[ 0], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 2], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[ 3], TaskItem(RECURSIVE, 2, false));
  EXPECT_EQ(order[ 4], TaskItem(QUITMESSAGELOOP, 3, true));
  EXPECT_EQ(order[ 5], TaskItem(QUITMESSAGELOOP, 3, false));
  EXPECT_EQ(order[ 6], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 7], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 8], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[ 9], TaskItem(RECURSIVE, 2, false));
  EXPECT_EQ(order[10], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[11], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[12], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[13], TaskItem(RECURSIVE, 2, false));
}

void RunTest_RecursiveSupport1(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  TaskList order;
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new RecursiveTask(2, &order, 1, true));
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new RecursiveTask(2, &order, 2, true));
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new QuitTask(&order, 3));

  MessageLoop::current()->Run();

  
  ASSERT_EQ(14U, order.size());
  EXPECT_EQ(order[ 0], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 2], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[ 3], TaskItem(RECURSIVE, 2, false));
  EXPECT_EQ(order[ 4], TaskItem(QUITMESSAGELOOP, 3, true));
  EXPECT_EQ(order[ 5], TaskItem(QUITMESSAGELOOP, 3, false));
  EXPECT_EQ(order[ 6], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 7], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 8], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[ 9], TaskItem(RECURSIVE, 2, false));
  EXPECT_EQ(order[10], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[11], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[12], TaskItem(RECURSIVE, 2, true));
  EXPECT_EQ(order[13], TaskItem(RECURSIVE, 2, false));
}

#if defined(OS_WIN)




void RunTest_RecursiveDenial2(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  Thread worker("RecursiveDenial2_worker");
  Thread::Options options;
  options.message_loop_type = message_loop_type;
  ASSERT_EQ(true, worker.StartWithOptions(options));
  TaskList order;
  ScopedHandle event(CreateEvent(NULL, FALSE, FALSE, NULL));
  worker.message_loop()->PostTask(FROM_HERE,
                                  new Recursive2Tasks(MessageLoop::current(),
                                                      event,
                                                      true,
                                                      &order,
                                                      false));
  
  WaitForSingleObject(event, INFINITE);
  MessageLoop::current()->Run();

  ASSERT_EQ(order.size(), 17);
  EXPECT_EQ(order[ 0], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 2], TaskItem(MESSAGEBOX, 2, true));
  EXPECT_EQ(order[ 3], TaskItem(MESSAGEBOX, 2, false));
  EXPECT_EQ(order[ 4], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[ 5], TaskItem(RECURSIVE, 3, false));
  
  
  EXPECT_EQ(order[ 6], TaskItem(ENDDIALOG, 4, true));
  EXPECT_EQ(order[ 7], TaskItem(QUITMESSAGELOOP, 5, true));
  EXPECT_EQ(order[ 8], TaskItem(QUITMESSAGELOOP, 5, false));
  EXPECT_EQ(order[ 9], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[10], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[11], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[12], TaskItem(RECURSIVE, 3, false));
  EXPECT_EQ(order[13], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[14], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[15], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[16], TaskItem(RECURSIVE, 3, false));
}



void RunTest_RecursiveSupport2(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  Thread worker("RecursiveSupport2_worker");
  Thread::Options options;
  options.message_loop_type = message_loop_type;
  ASSERT_EQ(true, worker.StartWithOptions(options));
  TaskList order;
  ScopedHandle event(CreateEvent(NULL, FALSE, FALSE, NULL));
  worker.message_loop()->PostTask(FROM_HERE,
                                  new Recursive2Tasks(MessageLoop::current(),
                                                      event,
                                                      false,
                                                      &order,
                                                      true));
  
  WaitForSingleObject(event, INFINITE);
  MessageLoop::current()->Run();

  ASSERT_EQ(order.size(), 18);
  EXPECT_EQ(order[ 0], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[ 2], TaskItem(MESSAGEBOX, 2, true));
  
  EXPECT_EQ(order[ 3], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[ 4], TaskItem(RECURSIVE, 3, false));
  EXPECT_EQ(order[ 5], TaskItem(ENDDIALOG, 4, true));
  EXPECT_EQ(order[ 6], TaskItem(ENDDIALOG, 4, false));
  EXPECT_EQ(order[ 7], TaskItem(MESSAGEBOX, 2, false));
  








  EXPECT_EQ(order[12], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[13], TaskItem(RECURSIVE, 3, false));
  EXPECT_EQ(order[14], TaskItem(RECURSIVE, 1, true));
  EXPECT_EQ(order[15], TaskItem(RECURSIVE, 1, false));
  EXPECT_EQ(order[16], TaskItem(RECURSIVE, 3, true));
  EXPECT_EQ(order[17], TaskItem(RECURSIVE, 3, false));
}

#endif  

class TaskThatPumps : public OrderedTasks {
 public:
  TaskThatPumps(TaskList* order, int cookie)
      : OrderedTasks(order, PUMPS, cookie) {
  }

  virtual void Run() {
    RunStart();
    bool old_state = MessageLoop::current()->NestableTasksAllowed();
    MessageLoop::current()->SetNestableTasksAllowed(true);
    MessageLoop::current()->RunAllPending();
    MessageLoop::current()->SetNestableTasksAllowed(old_state);
    RunEnd();
  }
};


void RunTest_NonNestableWithNoNesting(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  TaskList order;

  Task* task = new OrderedTasks(&order, 1);
  MessageLoop::current()->PostNonNestableTask(FROM_HERE, task);
  MessageLoop::current()->PostTask(FROM_HERE, new OrderedTasks(&order, 2));
  MessageLoop::current()->PostTask(FROM_HERE, new QuitTask(&order, 3));
  MessageLoop::current()->Run();

  
  ASSERT_EQ(6U, order.size());
  EXPECT_EQ(order[ 0], TaskItem(ORDERERD, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(ORDERERD, 1, false));
  EXPECT_EQ(order[ 2], TaskItem(ORDERERD, 2, true));
  EXPECT_EQ(order[ 3], TaskItem(ORDERERD, 2, false));
  EXPECT_EQ(order[ 4], TaskItem(QUITMESSAGELOOP, 3, true));
  EXPECT_EQ(order[ 5], TaskItem(QUITMESSAGELOOP, 3, false));
}


void RunTest_NonNestableInNestedLoop(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  TaskList order;

  MessageLoop::current()->PostTask(FROM_HERE,
                                   new TaskThatPumps(&order, 1));
  Task* task = new OrderedTasks(&order, 2);
  MessageLoop::current()->PostNonNestableTask(FROM_HERE, task);
  MessageLoop::current()->PostTask(FROM_HERE, new OrderedTasks(&order, 3));
  MessageLoop::current()->PostTask(FROM_HERE, new OrderedTasks(&order, 4));
  Task* non_nestable_quit = new QuitTask(&order, 5);
  MessageLoop::current()->PostNonNestableTask(FROM_HERE, non_nestable_quit);

  MessageLoop::current()->Run();

  
  ASSERT_EQ(10U, order.size());
  EXPECT_EQ(order[ 0], TaskItem(PUMPS, 1, true));
  EXPECT_EQ(order[ 1], TaskItem(ORDERERD, 3, true));
  EXPECT_EQ(order[ 2], TaskItem(ORDERERD, 3, false));
  EXPECT_EQ(order[ 3], TaskItem(ORDERERD, 4, true));
  EXPECT_EQ(order[ 4], TaskItem(ORDERERD, 4, false));
  EXPECT_EQ(order[ 5], TaskItem(PUMPS, 1, false));
  EXPECT_EQ(order[ 6], TaskItem(ORDERERD, 2, true));
  EXPECT_EQ(order[ 7], TaskItem(ORDERERD, 2, false));
  EXPECT_EQ(order[ 8], TaskItem(QUITMESSAGELOOP, 5, true));
  EXPECT_EQ(order[ 9], TaskItem(QUITMESSAGELOOP, 5, false));
}

#if defined(OS_WIN)

class DispatcherImpl : public MessageLoopForUI::Dispatcher {
 public:
  DispatcherImpl() : dispatch_count_(0) {}

  virtual bool Dispatch(const MSG& msg) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    return (++dispatch_count_ != 2);
  }

  int dispatch_count_;
};

void RunTest_Dispatcher(MessageLoop::Type message_loop_type) {
  MessageLoop loop(message_loop_type);

  class MyTask : public Task {
  public:
    virtual void Run() {
      PostMessage(NULL, WM_LBUTTONDOWN, 0, 0);
      PostMessage(NULL, WM_LBUTTONUP, 'A', 0);
    }
  };
  Task* task = new MyTask();
  MessageLoop::current()->PostDelayedTask(FROM_HERE, task, 100);
  DispatcherImpl dispatcher;
  MessageLoopForUI::current()->Run(&dispatcher);
  ASSERT_EQ(2, dispatcher.dispatch_count_);
}

class TestIOHandler : public MessageLoopForIO::IOHandler {
 public:
  TestIOHandler(const wchar_t* name, HANDLE signal, bool wait);

  virtual void OnIOCompleted(MessageLoopForIO::IOContext* context,
                             DWORD bytes_transfered, DWORD error);

  void Init();
  void WaitForIO();
  OVERLAPPED* context() { return &context_.overlapped; }
  DWORD size() { return sizeof(buffer_); }

 private:
  char buffer_[48];
  MessageLoopForIO::IOContext context_;
  HANDLE signal_;
  ScopedHandle file_;
  bool wait_;
};

TestIOHandler::TestIOHandler(const wchar_t* name, HANDLE signal, bool wait)
    : signal_(signal), wait_(wait) {
  memset(buffer_, 0, sizeof(buffer_));
  memset(&context_, 0, sizeof(context_));
  context_.handler = this;

  file_.Set(CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED, NULL));
  EXPECT_TRUE(file_.IsValid());
}

void TestIOHandler::Init() {
  MessageLoopForIO::current()->RegisterIOHandler(file_, this);

  DWORD read;
  EXPECT_FALSE(ReadFile(file_, buffer_, size(), &read, context()));
  EXPECT_EQ(ERROR_IO_PENDING, GetLastError());
  if (wait_)
    WaitForIO();
}

void TestIOHandler::OnIOCompleted(MessageLoopForIO::IOContext* context,
                                  DWORD bytes_transfered, DWORD error) {
  ASSERT_TRUE(context == &context_);
  ASSERT_TRUE(SetEvent(signal_));
}

void TestIOHandler::WaitForIO() {
  EXPECT_TRUE(MessageLoopForIO::current()->WaitForIOCompletion(300, this));
  EXPECT_TRUE(MessageLoopForIO::current()->WaitForIOCompletion(400, this));
}

class IOHandlerTask : public Task {
 public:
  explicit IOHandlerTask(TestIOHandler* handler) : handler_(handler) {}
  virtual void Run() {
    handler_->Init();
  }

 private:
  TestIOHandler* handler_;
};

void RunTest_IOHandler() {
  ScopedHandle callback_called(CreateEvent(NULL, TRUE, FALSE, NULL));
  ASSERT_TRUE(callback_called.IsValid());

  const wchar_t* kPipeName = L"\\\\.\\pipe\\iohandler_pipe";
  ScopedHandle server(CreateNamedPipe(kPipeName, PIPE_ACCESS_OUTBOUND, 0, 1,
                                      0, 0, 0, NULL));
  ASSERT_TRUE(server.IsValid());

  Thread thread("IOHandler test");
  Thread::Options options;
  options.message_loop_type = MessageLoop::TYPE_IO;
  ASSERT_TRUE(thread.StartWithOptions(options));

  MessageLoop* thread_loop = thread.message_loop();
  ASSERT_TRUE(NULL != thread_loop);

  TestIOHandler handler(kPipeName, callback_called, false);
  IOHandlerTask* task = new IOHandlerTask(&handler);
  thread_loop->PostTask(FROM_HERE, task);
  Sleep(100);  

  const char buffer[] = "Hello there!";
  DWORD written;
  EXPECT_TRUE(WriteFile(server, buffer, sizeof(buffer), &written, NULL));

  DWORD result = WaitForSingleObject(callback_called, 1000);
  EXPECT_EQ(WAIT_OBJECT_0, result);

  thread.Stop();
}

void RunTest_WaitForIO() {
  ScopedHandle callback1_called(CreateEvent(NULL, TRUE, FALSE, NULL));
  ScopedHandle callback2_called(CreateEvent(NULL, TRUE, FALSE, NULL));
  ASSERT_TRUE(callback1_called.IsValid());
  ASSERT_TRUE(callback2_called.IsValid());

  const wchar_t* kPipeName1 = L"\\\\.\\pipe\\iohandler_pipe1";
  const wchar_t* kPipeName2 = L"\\\\.\\pipe\\iohandler_pipe2";
  ScopedHandle server1(CreateNamedPipe(kPipeName1, PIPE_ACCESS_OUTBOUND, 0, 1,
                                       0, 0, 0, NULL));
  ScopedHandle server2(CreateNamedPipe(kPipeName2, PIPE_ACCESS_OUTBOUND, 0, 1,
                                       0, 0, 0, NULL));
  ASSERT_TRUE(server1.IsValid());
  ASSERT_TRUE(server2.IsValid());

  Thread thread("IOHandler test");
  Thread::Options options;
  options.message_loop_type = MessageLoop::TYPE_IO;
  ASSERT_TRUE(thread.StartWithOptions(options));

  MessageLoop* thread_loop = thread.message_loop();
  ASSERT_TRUE(NULL != thread_loop);

  TestIOHandler handler1(kPipeName1, callback1_called, false);
  TestIOHandler handler2(kPipeName2, callback2_called, true);
  IOHandlerTask* task1 = new IOHandlerTask(&handler1);
  IOHandlerTask* task2 = new IOHandlerTask(&handler2);
  thread_loop->PostTask(FROM_HERE, task1);
  Sleep(100);  
  thread_loop->PostTask(FROM_HERE, task2);
  Sleep(100);

  
  

  const char buffer[] = "Hello there!";
  DWORD written;
  EXPECT_TRUE(WriteFile(server1, buffer, sizeof(buffer), &written, NULL));
  Sleep(200);
  EXPECT_EQ(WAIT_TIMEOUT, WaitForSingleObject(callback1_called, 0)) <<
      "handler1 has not been called";

  EXPECT_TRUE(WriteFile(server2, buffer, sizeof(buffer), &written, NULL));

  HANDLE objects[2] = { callback1_called.Get(), callback2_called.Get() };
  DWORD result = WaitForMultipleObjects(2, objects, TRUE, 1000);
  EXPECT_EQ(WAIT_OBJECT_0, result);

  thread.Stop();
}

#endif  

}  






TEST(MessageLoopTest, PostTask) {
  RunTest_PostTask(MessageLoop::TYPE_DEFAULT);
  RunTest_PostTask(MessageLoop::TYPE_UI);
  RunTest_PostTask(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostTask_SEH) {
  RunTest_PostTask_SEH(MessageLoop::TYPE_DEFAULT);
  RunTest_PostTask_SEH(MessageLoop::TYPE_UI);
  RunTest_PostTask_SEH(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_Basic) {
  RunTest_PostDelayedTask_Basic(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_Basic(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_Basic(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_InDelayOrder) {
  RunTest_PostDelayedTask_InDelayOrder(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_InDelayOrder(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_InDelayOrder(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_InPostOrder) {
  RunTest_PostDelayedTask_InPostOrder(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_InPostOrder(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_InPostOrder(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_InPostOrder_2) {
  RunTest_PostDelayedTask_InPostOrder_2(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_InPostOrder_2(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_InPostOrder_2(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_InPostOrder_3) {
  RunTest_PostDelayedTask_InPostOrder_3(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_InPostOrder_3(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_InPostOrder_3(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, PostDelayedTask_SharedTimer) {
  RunTest_PostDelayedTask_SharedTimer(MessageLoop::TYPE_DEFAULT);
  RunTest_PostDelayedTask_SharedTimer(MessageLoop::TYPE_UI);
  RunTest_PostDelayedTask_SharedTimer(MessageLoop::TYPE_IO);
}

#if defined(OS_WIN)
TEST(MessageLoopTest, PostDelayedTask_SharedTimer_SubPump) {
  RunTest_PostDelayedTask_SharedTimer_SubPump();
}
#endif


#if 0
TEST(MessageLoopTest, EnsureTaskDeletion) {
  RunTest_EnsureTaskDeletion(MessageLoop::TYPE_DEFAULT);
  RunTest_EnsureTaskDeletion(MessageLoop::TYPE_UI);
  RunTest_EnsureTaskDeletion(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, EnsureTaskDeletion_Chain) {
  RunTest_EnsureTaskDeletion_Chain(MessageLoop::TYPE_DEFAULT);
  RunTest_EnsureTaskDeletion_Chain(MessageLoop::TYPE_UI);
  RunTest_EnsureTaskDeletion_Chain(MessageLoop::TYPE_IO);
}
#endif

#if defined(OS_WIN)
TEST(MessageLoopTest, Crasher) {
  RunTest_Crasher(MessageLoop::TYPE_DEFAULT);
  RunTest_Crasher(MessageLoop::TYPE_UI);
  RunTest_Crasher(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, CrasherNasty) {
  RunTest_CrasherNasty(MessageLoop::TYPE_DEFAULT);
  RunTest_CrasherNasty(MessageLoop::TYPE_UI);
  RunTest_CrasherNasty(MessageLoop::TYPE_IO);
}
#endif  

TEST(MessageLoopTest, Nesting) {
  RunTest_Nesting(MessageLoop::TYPE_DEFAULT);
  RunTest_Nesting(MessageLoop::TYPE_UI);
  RunTest_Nesting(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, RecursiveDenial1) {
  RunTest_RecursiveDenial1(MessageLoop::TYPE_DEFAULT);
  RunTest_RecursiveDenial1(MessageLoop::TYPE_UI);
  RunTest_RecursiveDenial1(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, RecursiveSupport1) {
  RunTest_RecursiveSupport1(MessageLoop::TYPE_DEFAULT);
  RunTest_RecursiveSupport1(MessageLoop::TYPE_UI);
  RunTest_RecursiveSupport1(MessageLoop::TYPE_IO);
}

#if defined(OS_WIN)
TEST(MessageLoopTest, RecursiveDenial2) {
  RunTest_RecursiveDenial2(MessageLoop::TYPE_DEFAULT);
  RunTest_RecursiveDenial2(MessageLoop::TYPE_UI);
  RunTest_RecursiveDenial2(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, RecursiveSupport2) {
  
  RunTest_RecursiveSupport2(MessageLoop::TYPE_UI);
}
#endif  

TEST(MessageLoopTest, NonNestableWithNoNesting) {
  RunTest_NonNestableWithNoNesting(MessageLoop::TYPE_DEFAULT);
  RunTest_NonNestableWithNoNesting(MessageLoop::TYPE_UI);
  RunTest_NonNestableWithNoNesting(MessageLoop::TYPE_IO);
}

TEST(MessageLoopTest, NonNestableInNestedLoop) {
  RunTest_NonNestableInNestedLoop(MessageLoop::TYPE_DEFAULT);
  RunTest_NonNestableInNestedLoop(MessageLoop::TYPE_UI);
  RunTest_NonNestableInNestedLoop(MessageLoop::TYPE_IO);
}

#if defined(OS_WIN)
TEST(MessageLoopTest, Dispatcher) {
  
  RunTest_Dispatcher(MessageLoop::TYPE_UI);
}

TEST(MessageLoopTest, IOHandler) {
  RunTest_IOHandler();
}

TEST(MessageLoopTest, WaitForIO) {
  RunTest_WaitForIO();
}
#endif  

#if defined(OS_POSIX)

namespace {

class QuitDelegate : public
    base::MessagePumpLibevent::Watcher {
 public:
  virtual void OnFileCanWriteWithoutBlocking(int fd) {
    MessageLoop::current()->Quit();
  }
  virtual void OnFileCanReadWithoutBlocking(int fd) {
    MessageLoop::current()->Quit();
  }
};

}  

TEST(MessageLoopTest, DISABLED_FileDescriptorWatcherOutlivesMessageLoop) {
  
  
  
  
  

  
  
  
  int pipefds[2];
  int err = pipe(pipefds);
  ASSERT_TRUE(err == 0);
  int fd = pipefds[1];
  {
    
    base::MessagePumpLibevent::FileDescriptorWatcher controller;
    {
      MessageLoopForIO message_loop;

      QuitDelegate delegate;
      message_loop.WatchFileDescriptor(fd,
          true, MessageLoopForIO::WATCH_WRITE, &controller, &delegate);
      
    }
  }
  close(pipefds[0]);
  close(pipefds[1]);
}

TEST(MessageLoopTest, FileDescriptorWatcherDoubleStop) {
  
  
  int pipefds[2];
  int err = pipe(pipefds);
  ASSERT_TRUE(err == 0);
  int fd = pipefds[1];
  {
    
    MessageLoopForIO message_loop;
    {
      base::MessagePumpLibevent::FileDescriptorWatcher controller;

      QuitDelegate delegate;
      message_loop.WatchFileDescriptor(fd,
          true, MessageLoopForIO::WATCH_WRITE, &controller, &delegate);
      controller.StopWatchingFileDescriptor();
    }
  }
  close(pipefds[0]);
  close(pipefds[1]);
}

#endif  
