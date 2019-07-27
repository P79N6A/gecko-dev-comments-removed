









#if defined(WEBRTC_POSIX)
#include <sys/time.h>
#endif  



#include <iostream>

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif  

#include "webrtc/base/common.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/task.h"
#include "webrtc/base/taskrunner.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

static int64 GetCurrentTime() {
  return static_cast<int64>(Time()) * 10000;
}


#define STUCK_TASK_COUNT 5
#define HAPPY_TASK_COUNT 20






class IdTimeoutTask : public Task, public sigslot::has_slots<> {
 public:
  explicit IdTimeoutTask(TaskParent *parent) : Task(parent) {
    SignalTimeout.connect(this, &IdTimeoutTask::OnLocalTimeout);
  }

  sigslot::signal1<const int> SignalTimeoutId;
  sigslot::signal1<const int> SignalDoneId;

  virtual int ProcessStart() {
    return STATE_RESPONSE;
  }

  void OnLocalTimeout() {
    SignalTimeoutId(unique_id());
  }

 protected:
  virtual void Stop() {
    SignalDoneId(unique_id());
    Task::Stop();
  }
};

class StuckTask : public IdTimeoutTask {
 public:
  explicit StuckTask(TaskParent *parent) : IdTimeoutTask(parent) {}
  virtual int ProcessStart() {
    return STATE_BLOCKED;
  }
};

class HappyTask : public IdTimeoutTask {
 public:
  explicit HappyTask(TaskParent *parent) : IdTimeoutTask(parent) {
    time_to_perform_ = rand() % (STUCK_TASK_COUNT / 2);
  }
  virtual int ProcessStart() {
    if (ElapsedTime() > (time_to_perform_ * 1000 * 10000))
      return STATE_RESPONSE;
    else
      return STATE_BLOCKED;
  }

 private:
  int time_to_perform_;
};




class MyTaskRunner : public TaskRunner {
 public:
  virtual void WakeTasks() { RunTasks(); }
  virtual int64 CurrentTime() {
    return GetCurrentTime();
  }

  bool timeout_change() const {
    return timeout_change_;
  }

  void clear_timeout_change() {
    timeout_change_ = false;
  }
 protected:
  virtual void OnTimeoutChange() {
    timeout_change_ = true;
  }
  bool timeout_change_;
};












class TaskTest : public sigslot::has_slots<> {
 public:
  TaskTest() {}

  
  ~TaskTest() {}

  void Start() {
    
    for (int i = 0; i < STUCK_TASK_COUNT; ++i) {
      stuck_[i].task_ = new StuckTask(&task_runner_);
      stuck_[i].task_->SignalTimeoutId.connect(this,
                                               &TaskTest::OnTimeoutStuck);
      stuck_[i].timed_out_ = false;
      stuck_[i].xlat_ = stuck_[i].task_->unique_id();
      stuck_[i].task_->set_timeout_seconds(i + 1);
      LOG(LS_INFO) << "Task " << stuck_[i].xlat_ << " created with timeout "
                   << stuck_[i].task_->timeout_seconds();
    }

    for (int i = 0; i < HAPPY_TASK_COUNT; ++i) {
      happy_[i].task_ = new HappyTask(&task_runner_);
      happy_[i].task_->SignalTimeoutId.connect(this,
                                               &TaskTest::OnTimeoutHappy);
      happy_[i].task_->SignalDoneId.connect(this,
                                            &TaskTest::OnDoneHappy);
      happy_[i].timed_out_ = false;
      happy_[i].xlat_ = happy_[i].task_->unique_id();
    }

    
    int stuck_index = 0;
    int happy_index = 0;
    for (int i = 0; i < STUCK_TASK_COUNT + HAPPY_TASK_COUNT; ++i) {
      if ((stuck_index < STUCK_TASK_COUNT) &&
          (happy_index < HAPPY_TASK_COUNT)) {
        if (rand() % 2 == 1) {
          stuck_[stuck_index++].task_->Start();
        } else {
          happy_[happy_index++].task_->Start();
        }
      } else if (stuck_index < STUCK_TASK_COUNT) {
        stuck_[stuck_index++].task_->Start();
      } else {
        happy_[happy_index++].task_->Start();
      }
    }

    for (int i = 0; i < STUCK_TASK_COUNT; ++i) {
      std::cout << "Stuck task #" << i << " timeout is " <<
          stuck_[i].task_->timeout_seconds() << " at " <<
          stuck_[i].task_->timeout_time() << std::endl;
    }

    
    ASSERT_EQ(STUCK_TASK_COUNT, stuck_index);
    ASSERT_EQ(HAPPY_TASK_COUNT, happy_index);

    
    LOG(LS_INFO) << "Running tasks";
    task_runner_.RunTasks();

    std::cout << "Start time is " << GetCurrentTime() << std::endl;

    
    for (int i = 0; !task_runner_.AllChildrenDone() && i < STUCK_TASK_COUNT;
         ++i) {
      Thread::Current()->ProcessMessages(1000);
      for (int j = 0; j < HAPPY_TASK_COUNT; ++j) {
        if (happy_[j].task_) {
          happy_[j].task_->Wake();
        }
      }
      LOG(LS_INFO) << "Polling tasks";
      task_runner_.PollTasks();
    }

    
    
    
    
    std::cout << "End time is " << GetCurrentTime() << std::endl;
  }

  void OnTimeoutStuck(const int id) {
    LOG(LS_INFO) << "Timed out task " << id;

    int i;
    for (i = 0; i < STUCK_TASK_COUNT; ++i) {
      if (stuck_[i].xlat_ == id) {
        stuck_[i].timed_out_ = true;
        stuck_[i].task_ = NULL;
        break;
      }
    }

    
    
    EXPECT_LT(i, STUCK_TASK_COUNT);
  }

  void OnTimeoutHappy(const int id) {
    int i;
    for (i = 0; i < HAPPY_TASK_COUNT; ++i) {
      if (happy_[i].xlat_ == id) {
        happy_[i].timed_out_ = true;
        happy_[i].task_ = NULL;
        break;
      }
    }

    
    
    EXPECT_LT(i, HAPPY_TASK_COUNT);
  }

  void OnDoneHappy(const int id) {
    int i;
    for (i = 0; i < HAPPY_TASK_COUNT; ++i) {
      if (happy_[i].xlat_ == id) {
        happy_[i].task_ = NULL;
        break;
      }
    }

    
    
    EXPECT_LT(i, HAPPY_TASK_COUNT);
  }

  void check_passed() {
    EXPECT_TRUE(task_runner_.AllChildrenDone());

    
    for (int i = 0; i < HAPPY_TASK_COUNT; ++i) {
      EXPECT_FALSE(happy_[i].timed_out_);
    }

    
    for (int i = 0; i < STUCK_TASK_COUNT; ++i) {
      EXPECT_TRUE(stuck_[i].timed_out_);
      if (!stuck_[i].timed_out_) {
        std::cout << "Stuck task #" << i << " timeout is at "
            << stuck_[i].task_->timeout_time() << std::endl;        
      }
    }

    std::cout.flush();
  }

 private:
  struct TaskInfo {
    IdTimeoutTask *task_;
    bool timed_out_;
    int xlat_;
  };

  MyTaskRunner task_runner_;
  TaskInfo stuck_[STUCK_TASK_COUNT];
  TaskInfo happy_[HAPPY_TASK_COUNT];
};

TEST(start_task_test, Timeout) {
  TaskTest task_test;
  task_test.Start();
  task_test.check_passed();
}



class AbortTask : public Task {
 public:
  explicit AbortTask(TaskParent *parent) : Task(parent) {
    set_timeout_seconds(1);
  }

  virtual int ProcessStart() {
    Abort();
    return STATE_NEXT;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(AbortTask);
};

class TaskAbortTest : public sigslot::has_slots<> {
 public:
  TaskAbortTest() {}

  
  ~TaskAbortTest() {}

  void Start() {
    Task *abort_task = new AbortTask(&task_runner_);
    abort_task->SignalTimeout.connect(this, &TaskAbortTest::OnTimeout);
    abort_task->Start();

    
    task_runner_.RunTasks();
  }

 private:
  void OnTimeout() {
    FAIL() << "Task timed out instead of aborting.";
  }

  MyTaskRunner task_runner_;
  DISALLOW_EVIL_CONSTRUCTORS(TaskAbortTest);
};

TEST(start_task_test, Abort) {
  TaskAbortTest abort_test;
  abort_test.Start();
}




class SetBoolOnDeleteTask : public Task {
 public:
  SetBoolOnDeleteTask(TaskParent *parent, bool *set_when_deleted)
    : Task(parent),
      set_when_deleted_(set_when_deleted) {
    EXPECT_TRUE(NULL != set_when_deleted);
    EXPECT_FALSE(*set_when_deleted);
  }

  virtual ~SetBoolOnDeleteTask() {
    *set_when_deleted_ = true;
  }

  virtual int ProcessStart() {
    return STATE_BLOCKED;
  }

 private:
  bool* set_when_deleted_;
  DISALLOW_EVIL_CONSTRUCTORS(SetBoolOnDeleteTask);
};

class AbortShouldWakeTest : public sigslot::has_slots<> {
 public:
  AbortShouldWakeTest() {}

  
  ~AbortShouldWakeTest() {}

  void Start() {
    bool task_deleted = false;
    Task *task_to_abort = new SetBoolOnDeleteTask(&task_runner_, &task_deleted);
    task_to_abort->Start();

    
    
    task_to_abort->Abort();
    EXPECT_TRUE(task_deleted);

    if (!task_deleted) {
      
      
      task_runner_.RunTasks();
    }
  }

 private:
  void OnTimeout() {
    FAIL() << "Task timed out instead of aborting.";
  }

  MyTaskRunner task_runner_;
  DISALLOW_EVIL_CONSTRUCTORS(AbortShouldWakeTest);
};

TEST(start_task_test, AbortShouldWake) {
  AbortShouldWakeTest abort_should_wake_test;
  abort_should_wake_test.Start();
}




class TimeoutChangeTest : public sigslot::has_slots<> {
 public:
  TimeoutChangeTest()
    : task_count_(ARRAY_SIZE(stuck_tasks_)) {}

  
  ~TimeoutChangeTest() {}

  void Start() {
    for (int i = 0; i < task_count_; ++i) {
      stuck_tasks_[i] = new StuckTask(&task_runner_);
      stuck_tasks_[i]->set_timeout_seconds(i + 2);
      stuck_tasks_[i]->SignalTimeoutId.connect(this,
                                               &TimeoutChangeTest::OnTimeoutId);
    }

    for (int i = task_count_ - 1; i >= 0; --i) {
      stuck_tasks_[i]->Start();
    }
    task_runner_.clear_timeout_change();

    
    

    stuck_tasks_[0]->set_timeout_seconds(2);
    
    
    EXPECT_FALSE(task_runner_.timeout_change());
    task_runner_.clear_timeout_change();

    stuck_tasks_[0]->set_timeout_seconds(1);
    
    
    EXPECT_TRUE(task_runner_.timeout_change());
    task_runner_.clear_timeout_change();

    stuck_tasks_[1]->set_timeout_seconds(2);
    
    
    EXPECT_FALSE(task_runner_.timeout_change());
    task_runner_.clear_timeout_change();

    while (task_count_ > 0) {
      int previous_count = task_count_;
      task_runner_.PollTasks();
      if (previous_count != task_count_) {
        
        
        
        EXPECT_TRUE(task_runner_.timeout_change());
        task_runner_.clear_timeout_change();
      }
      Thread::Current()->socketserver()->Wait(500, false);
    }
  }

 private:
  void OnTimeoutId(const int id) {
    for (int i = 0; i < ARRAY_SIZE(stuck_tasks_); ++i) {
      if (stuck_tasks_[i] && stuck_tasks_[i]->unique_id() == id) {
        task_count_--;
        stuck_tasks_[i] = NULL;
        break;
      }
    }
  }

  MyTaskRunner task_runner_;
  StuckTask* (stuck_tasks_[3]);
  int task_count_;
  DISALLOW_EVIL_CONSTRUCTORS(TimeoutChangeTest);
};

TEST(start_task_test, TimeoutChange) {
  TimeoutChangeTest timeout_change_test;
  timeout_change_test.Start();
}

class DeleteTestTaskRunner : public TaskRunner {
 public:
  DeleteTestTaskRunner() {
  }
  virtual void WakeTasks() { }
  virtual int64 CurrentTime() {
    return GetCurrentTime();
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(DeleteTestTaskRunner);
};

TEST(unstarted_task_test, DeleteTask) {
  
  
  DeleteTestTaskRunner task_runner;
  HappyTask* happy_task = new HappyTask(&task_runner);
  happy_task->Start();

  
  HappyTask* child_happy_task = new HappyTask(happy_task);
  delete child_happy_task;

  
  task_runner.RunTasks();
}

TEST(unstarted_task_test, DoNotDeleteTask1) {
  
  
  
  DeleteTestTaskRunner task_runner;
  HappyTask* happy_task = new HappyTask(&task_runner);
  happy_task->Start();

  HappyTask* child_happy_task = new HappyTask(happy_task);
  child_happy_task->Start();

  
}

TEST(unstarted_task_test, DoNotDeleteTask2) {
  
  
  
  DeleteTestTaskRunner task_runner;
  HappyTask* happy_task = new HappyTask(&task_runner);
  happy_task->Start();

  
  
  
  new HappyTask(happy_task);

  
  task_runner.RunTasks();
}

}  
