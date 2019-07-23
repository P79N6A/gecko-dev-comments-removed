





#include <time.h>
#include <algorithm>
#include <vector>

#include "base/condition_variable.h"
#include "base/logging.h"
#include "base/platform_thread.h"
#include "base/scoped_ptr.h"
#include "base/spin_wait.h"
#include "base/thread_collision_warner.h"
#include "base/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::TimeDelta;
using base::TimeTicks;

namespace {




class ConditionVariableTest : public PlatformTest {
 public:
  const TimeDelta kZeroMs;
  const TimeDelta kTenMs;
  const TimeDelta kThirtyMs;
  const TimeDelta kFortyFiveMs;
  const TimeDelta kSixtyMs;
  const TimeDelta kOneHundredMs;

  explicit ConditionVariableTest()
    : kZeroMs(TimeDelta::FromMilliseconds(0)),
      kTenMs(TimeDelta::FromMilliseconds(10)),
      kThirtyMs(TimeDelta::FromMilliseconds(30)),
      kFortyFiveMs(TimeDelta::FromMilliseconds(45)),
      kSixtyMs(TimeDelta::FromMilliseconds(60)),
      kOneHundredMs(TimeDelta::FromMilliseconds(100)) {
  }
};















class WorkQueue : public PlatformThread::Delegate {
 public:
  explicit WorkQueue(int thread_count);
  ~WorkQueue();

  
  void ThreadMain();

  
  
  
  int GetThreadId();  
  bool EveryIdWasAllocated() const;  
  TimeDelta GetAnAssignment(int thread_id);  
  void WorkIsCompleted(int thread_id);

  int task_count() const;
  bool allow_help_requests() const;  
  bool shutdown() const;  

  void thread_shutting_down();


  
  
  Lock* lock();

  ConditionVariable* work_is_available();
  ConditionVariable* all_threads_have_ids();
  ConditionVariable* no_more_tasks();

  
  
  
  void ResetHistory();
  int GetMinCompletionsByWorkerThread() const;
  int GetMaxCompletionsByWorkerThread() const;
  int GetNumThreadsTakingAssignments() const;
  int GetNumThreadsCompletingTasks() const;
  int GetNumberOfCompletedTasks() const;
  TimeDelta GetWorkTime() const;

  void SetWorkTime(TimeDelta delay);
  void SetTaskCount(int count);
  void SetAllowHelp(bool allow);

  
  void SetShutdown();

  
  
  
  bool ThreadSafeCheckShutdown(int thread_count);

 private:
  
  Lock lock_;
  ConditionVariable work_is_available_;  

  
  ConditionVariable all_threads_have_ids_;  
  ConditionVariable no_more_tasks_;  

  const int thread_count_;
  scoped_array<PlatformThreadHandle> thread_handles_;
  std::vector<int> assignment_history_;  
  std::vector<int> completion_history_;  
  int thread_started_counter_;  
  int shutdown_task_count_;  
  int task_count_;  
  TimeDelta worker_delay_;  
  bool allow_help_requests_;  
  bool shutdown_;  

  DFAKE_MUTEX(locked_methods_);
};





TEST_F(ConditionVariableTest, StartupShutdownTest) {
  Lock lock;

  
  {
    ConditionVariable cv1(&lock);
  }  

  
  ConditionVariable cv(&lock);

  lock.Acquire();
  cv.TimedWait(kTenMs);  
  cv.TimedWait(kTenMs);  
  lock.Release();

  lock.Acquire();
  cv.TimedWait(kTenMs);  
  cv.TimedWait(kTenMs);  
  cv.TimedWait(kTenMs);  
  lock.Release();
}  

TEST_F(ConditionVariableTest, TimeoutTest) {
  Lock lock;
  ConditionVariable cv(&lock);
  lock.Acquire();

  TimeTicks start = TimeTicks::Now();
  const TimeDelta WAIT_TIME = TimeDelta::FromMilliseconds(300);
  
  const TimeDelta FUDGE_TIME = TimeDelta::FromMilliseconds(50);

  cv.TimedWait(WAIT_TIME + FUDGE_TIME);
  TimeDelta duration = TimeTicks::Now() - start;
  
  
  EXPECT_TRUE(duration >= WAIT_TIME);

  lock.Release();
}



TEST_F(ConditionVariableTest, DISABLED_MultiThreadConsumerTest) {
  const int kThreadCount = 10;
  WorkQueue queue(kThreadCount);  

  const int kTaskCount = 10;  

  base::Time start_time;  

  {
    AutoLock auto_lock(*queue.lock());
    while (!queue.EveryIdWasAllocated())
      queue.all_threads_have_ids()->Wait();
  }

  
  
  
  PlatformThread::Sleep(100);

  {
    
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(0, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(0, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(0, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(kTaskCount);
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(false);

    start_time = base::Time::Now();
  }

  queue.work_is_available()->Signal();  


  {
    
    AutoLock auto_lock(*queue.lock());
    while(queue.task_count())
      queue.no_more_tasks()->Wait();
    
    
    EXPECT_LE(queue.GetWorkTime().InMilliseconds() * (kTaskCount - 1),
              (base::Time::Now() - start_time).InMilliseconds());

    EXPECT_EQ(1, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(1, queue.GetNumThreadsCompletingTasks());
    EXPECT_LE(kTaskCount - 1, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_LE(kTaskCount - 1, queue.GetNumberOfCompletedTasks());
  }

  
  while (1) {
    {
      AutoLock auto_lock(*queue.lock());
      if (kTaskCount == queue.GetNumberOfCompletedTasks())
        break;
    }
    PlatformThread::Sleep(30);  
  }

  {
    
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(1, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(1, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(kTaskCount, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(kTaskCount, queue.GetNumberOfCompletedTasks());

    
    
    queue.ResetHistory();
    queue.SetTaskCount(kTaskCount);
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(true);

    start_time = base::Time::Now();
  }

  queue.work_is_available()->Signal();  
  
  while (1) {
    {
      AutoLock auto_lock(*queue.lock());
      if (kTaskCount == queue.GetNumberOfCompletedTasks())
        break;
    }
    PlatformThread::Sleep(30);  
  }

  {
    
    AutoLock auto_lock(*queue.lock());
    while(queue.task_count())
      queue.no_more_tasks()->Wait();
    
    
    
    EXPECT_GT(queue.GetWorkTime().InMilliseconds() * (kTaskCount - 1),
              (base::Time::Now() - start_time).InMilliseconds());

    
    
    EXPECT_LE(2, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(kTaskCount, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(3);
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(false);
  }
  queue.work_is_available()->Broadcast();  
  
  PlatformThread::Sleep(45);

  {
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(3, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(3, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(1, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(3, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(3);
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(true);  
  }
  queue.work_is_available()->Broadcast();  
  
  PlatformThread::Sleep(100);

  {
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(3, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(3, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(1, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(3, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(20);
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(true);
  }
  queue.work_is_available()->Signal();  
  
  PlatformThread::Sleep(100);  

  {
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(10, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(10, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(2, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(2, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(20, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(20);  
    queue.SetWorkTime(kThirtyMs);
    queue.SetAllowHelp(true);
  }
  queue.work_is_available()->Broadcast();
  
  PlatformThread::Sleep(100);  

  {
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(10, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(10, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(2, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(2, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(20, queue.GetNumberOfCompletedTasks());

    queue.SetShutdown();
  }
  queue.work_is_available()->Broadcast();  

  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(1),
                                   queue.ThreadSafeCheckShutdown(kThreadCount));
  PlatformThread::Sleep(10);  
}

TEST_F(ConditionVariableTest, LargeFastTaskTest) {
  const int kThreadCount = 200;
  WorkQueue queue(kThreadCount);  

  Lock private_lock;  
  AutoLock private_held_lock(private_lock);
  ConditionVariable private_cv(&private_lock);

  {
    AutoLock auto_lock(*queue.lock());
    while (!queue.EveryIdWasAllocated())
      queue.all_threads_have_ids()->Wait();
  }

  
  private_cv.TimedWait(kThirtyMs);

  {
    
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(0, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(0, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_EQ(0, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetMinCompletionsByWorkerThread());
    EXPECT_EQ(0, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(20 * kThreadCount);
    queue.SetWorkTime(kFortyFiveMs);
    queue.SetAllowHelp(false);
  }
  queue.work_is_available()->Broadcast();  
  
  {
    AutoLock auto_lock(*queue.lock());
    while (queue.task_count() != 0)
      queue.no_more_tasks()->Wait();
  }

  
  
  
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(1),
                                    20 * kThreadCount ==
                                      queue.GetNumberOfCompletedTasks());

  {
    
    
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(kThreadCount, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(kThreadCount, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_LE(20, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(20 * kThreadCount, queue.GetNumberOfCompletedTasks());

    
    queue.ResetHistory();
    queue.SetTaskCount(kThreadCount * 4);
    queue.SetWorkTime(kFortyFiveMs);
    queue.SetAllowHelp(true);  
  }
  queue.work_is_available()->Signal();  

  
  {
    AutoLock auto_lock(*queue.lock());
    while (queue.task_count() != 0)
      queue.no_more_tasks()->Wait();
  }

  
  
  
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(1),
                                    4 * kThreadCount ==
                                      queue.GetNumberOfCompletedTasks());

  {
    
    
    AutoLock auto_lock(*queue.lock());
    EXPECT_EQ(kThreadCount, queue.GetNumThreadsTakingAssignments());
    EXPECT_EQ(kThreadCount, queue.GetNumThreadsCompletingTasks());
    EXPECT_EQ(0, queue.task_count());
    EXPECT_LE(4, queue.GetMaxCompletionsByWorkerThread());
    EXPECT_EQ(4 * kThreadCount, queue.GetNumberOfCompletedTasks());

    queue.SetShutdown();
  }
  queue.work_is_available()->Broadcast();  

  
  SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(TimeDelta::FromMinutes(1),
                                   queue.ThreadSafeCheckShutdown(kThreadCount));
  PlatformThread::Sleep(10);  
}





WorkQueue::WorkQueue(int thread_count)
  : lock_(),
    work_is_available_(&lock_),
    all_threads_have_ids_(&lock_),
    no_more_tasks_(&lock_),
    thread_count_(thread_count),
    thread_handles_(new PlatformThreadHandle[thread_count]),
    assignment_history_(thread_count),
    completion_history_(thread_count),
    thread_started_counter_(0),
    shutdown_task_count_(0),
    task_count_(0),
    allow_help_requests_(false),
    shutdown_(false) {
  EXPECT_GE(thread_count_, 1);
  ResetHistory();
  SetTaskCount(0);
  SetWorkTime(TimeDelta::FromMilliseconds(30));

  for (int i = 0; i < thread_count_; ++i) {
    PlatformThreadHandle pth;
    EXPECT_TRUE(PlatformThread::Create(0, this, &pth));
    thread_handles_[i] = pth;
  }
}

WorkQueue::~WorkQueue() {
  {
    AutoLock auto_lock(lock_);
    SetShutdown();
  }
  work_is_available_.Broadcast();  

  for (int i = 0; i < thread_count_; ++i) {
    PlatformThread::Join(thread_handles_[i]);
  }
}

int WorkQueue::GetThreadId() {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  DCHECK(!EveryIdWasAllocated());
  return thread_started_counter_++;  
}

bool WorkQueue::EveryIdWasAllocated() const {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  return thread_count_ == thread_started_counter_;
}

TimeDelta WorkQueue::GetAnAssignment(int thread_id) {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  DCHECK_LT(0, task_count_);
  assignment_history_[thread_id]++;
  if (0 == --task_count_) {
    no_more_tasks_.Signal();
  }
  return worker_delay_;
}

void WorkQueue::WorkIsCompleted(int thread_id) {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  completion_history_[thread_id]++;
}

int WorkQueue::task_count() const {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  return task_count_;
}

bool WorkQueue::allow_help_requests() const {
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  return allow_help_requests_;
}

bool WorkQueue::shutdown() const {
  lock_.AssertAcquired();
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  return shutdown_;
}




bool WorkQueue::ThreadSafeCheckShutdown(int thread_count) {
  bool all_shutdown;
  AutoLock auto_lock(lock_);
  {
    
    DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
    all_shutdown = (shutdown_task_count_ == thread_count);
  }
  return all_shutdown;
}

void WorkQueue::thread_shutting_down() {
  lock_.AssertAcquired();
  DFAKE_SCOPED_RECURSIVE_LOCK(locked_methods_);
  shutdown_task_count_++;
}

Lock* WorkQueue::lock() {
  return &lock_;
}

ConditionVariable* WorkQueue::work_is_available() {
  return &work_is_available_;
}

ConditionVariable* WorkQueue::all_threads_have_ids() {
  return &all_threads_have_ids_;
}

ConditionVariable* WorkQueue::no_more_tasks() {
  return &no_more_tasks_;
}

void WorkQueue::ResetHistory() {
  for (int i = 0; i < thread_count_; ++i) {
    assignment_history_[i] = 0;
    completion_history_[i] = 0;
  }
}

int WorkQueue::GetMinCompletionsByWorkerThread() const {
  int minumum = completion_history_[0];
  for (int i = 0; i < thread_count_; ++i)
    minumum = std::min(minumum, completion_history_[i]);
  return minumum;
}

int WorkQueue::GetMaxCompletionsByWorkerThread() const {
  int maximum = completion_history_[0];
  for (int i = 0; i < thread_count_; ++i)
    maximum = std::max(maximum, completion_history_[i]);
  return maximum;
}

int WorkQueue::GetNumThreadsTakingAssignments() const {
  int count = 0;
  for (int i = 0; i < thread_count_; ++i)
    if (assignment_history_[i])
      count++;
  return count;
}

int WorkQueue::GetNumThreadsCompletingTasks() const {
  int count = 0;
  for (int i = 0; i < thread_count_; ++i)
    if (completion_history_[i])
      count++;
  return count;
}

int WorkQueue::GetNumberOfCompletedTasks() const {
  int total = 0;
  for (int i = 0; i < thread_count_; ++i)
    total += completion_history_[i];
  return total;
}

TimeDelta WorkQueue::GetWorkTime() const {
  return worker_delay_;
}

void WorkQueue::SetWorkTime(TimeDelta delay) {
  worker_delay_ = delay;
}

void WorkQueue::SetTaskCount(int count) {
  task_count_ = count;
}

void WorkQueue::SetAllowHelp(bool allow) {
  allow_help_requests_ = allow;
}

void WorkQueue::SetShutdown() {
  lock_.AssertAcquired();
  shutdown_ = true;
}



















void WorkQueue::ThreadMain() {
  int thread_id;
  {
    AutoLock auto_lock(lock_);
    thread_id = GetThreadId();
    if (EveryIdWasAllocated())
      all_threads_have_ids()->Signal();  
  }

  Lock private_lock;  
  while (1) {  
    TimeDelta work_time;
    bool could_use_help;
    {
      AutoLock auto_lock(lock_);
      while (0 == task_count() && !shutdown()) {
        work_is_available()->Wait();
      }
      if (shutdown()) {
        
        thread_shutting_down();
        return;  
      }
      
      work_time = GetAnAssignment(thread_id);
      could_use_help = (task_count() > 0) && allow_help_requests();
    }  

    
    if (could_use_help)
      work_is_available()->Signal();  

    if (work_time > TimeDelta::FromMilliseconds(0)) {
      
      
      AutoLock auto_lock(private_lock);
      ConditionVariable private_cv(&private_lock);
      private_cv.TimedWait(work_time);  
    }

    {
      AutoLock auto_lock(lock_);
      
      WorkIsCompleted(thread_id);
    }
  }
}

}  
