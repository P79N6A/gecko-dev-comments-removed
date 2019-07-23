



#include "base/idle_timer.h"
#include "base/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using base::IdleTimer;






const int kSafeTestIntervalMs = 500;

namespace {



static Time mock_timer_started;

bool MockIdleTimeSource(int32 *milliseconds_interval_since_last_event) {
  TimeDelta delta = Time::Now() - mock_timer_started;
  *milliseconds_interval_since_last_event =
      static_cast<int32>(delta.InMilliseconds());
  return true;
}


class TestIdleTask : public IdleTimer {
 public:
  TestIdleTask(bool repeat)
      : IdleTimer(TimeDelta::FromMilliseconds(kSafeTestIntervalMs), repeat),
        idle_counter_(0) {
        set_idle_time_source(MockIdleTimeSource);
  }

  int get_idle_counter() { return idle_counter_; }

  virtual void OnIdle() {
    idle_counter_++;
  }

 private:
  int idle_counter_;
};


class TestFinishedTask {
 public:
  TestFinishedTask() {}
  void Run() {
    MessageLoop::current()->Quit();
  }
};


class ResetIdleTask {
 public:
  ResetIdleTask() {}
  void Run() {
    mock_timer_started = Time::Now();
  }
};

class IdleTimerTest : public testing::Test {
 private:
  
  MessageLoopForUI message_loop_;
};






TEST_F(IdleTimerTest, NoRepeatIdle) {
  
  
  

  mock_timer_started = Time::Now();
  TestIdleTask test_task(false);

  TestFinishedTask finish_task;
  base::OneShotTimer<TestFinishedTask> timer;
  timer.Start(TimeDelta::FromMilliseconds(2 * kSafeTestIntervalMs),
      &finish_task, &TestFinishedTask::Run);

  test_task.Start();
  MessageLoop::current()->Run();

  EXPECT_EQ(test_task.get_idle_counter(), 1);
}

TEST_F(IdleTimerTest, NoRepeatFlipIdleOnce) {
  
  
  
  

  mock_timer_started = Time::Now();
  TestIdleTask test_task(false);

  TestFinishedTask finish_task;
  ResetIdleTask reset_task;

  base::OneShotTimer<TestFinishedTask> t1;
  t1.Start(TimeDelta::FromMilliseconds(10 * kSafeTestIntervalMs), &finish_task,
           &TestFinishedTask::Run);

  base::OneShotTimer<ResetIdleTask> t2;
  t2.Start(TimeDelta::FromMilliseconds(4 * kSafeTestIntervalMs), &reset_task,
           &ResetIdleTask::Run);

  test_task.Start();
  MessageLoop::current()->Run();

  EXPECT_EQ(test_task.get_idle_counter(), 2);
}

TEST_F(IdleTimerTest, NoRepeatNotIdle) {
  
  
  
  

  mock_timer_started = Time::Now();
  TestIdleTask test_task(false);

  TestFinishedTask finish_task;
  ResetIdleTask reset_task;

  base::OneShotTimer<TestFinishedTask> t;
  t.Start(TimeDelta::FromMilliseconds(10 * kSafeTestIntervalMs), &finish_task,
          &TestFinishedTask::Run);

  base::RepeatingTimer<ResetIdleTask> reset_timer;
  reset_timer.Start(TimeDelta::FromMilliseconds(50), &reset_task,
                    &ResetIdleTask::Run);

  test_task.Start();

  MessageLoop::current()->Run();

  reset_timer.Stop();

  EXPECT_EQ(test_task.get_idle_counter(), 0);
}







TEST_F(IdleTimerTest, Repeat) {
  
  
  
  mock_timer_started = Time::Now();
  TestIdleTask test_task(true);

  TestFinishedTask finish_task;

  base::OneShotTimer<TestFinishedTask> t;
  t.Start(TimeDelta::FromMilliseconds(kSafeTestIntervalMs * 3), &finish_task,
          &TestFinishedTask::Run);

  test_task.Start();
  MessageLoop::current()->Run();

  
  
  EXPECT_GE(test_task.get_idle_counter(), 2);
  EXPECT_LE(test_task.get_idle_counter(), 3);
}

TEST_F(IdleTimerTest, RepeatIdleReset) {
  
  
  
  
  mock_timer_started = Time::Now();
  TestIdleTask test_task(true);

  ResetIdleTask reset_task;
  TestFinishedTask finish_task;

  base::OneShotTimer<TestFinishedTask> t1;
  t1.Start(TimeDelta::FromMilliseconds(10 * kSafeTestIntervalMs), &finish_task,
           &TestFinishedTask::Run);

  base::OneShotTimer<ResetIdleTask> t2;
  t2.Start(TimeDelta::FromMilliseconds(5 * kSafeTestIntervalMs), &reset_task,
           &ResetIdleTask::Run);

  test_task.Start();
  MessageLoop::current()->Run();

  
  
  
  EXPECT_GE(test_task.get_idle_counter(), 8);
  EXPECT_LE(test_task.get_idle_counter(), 10);
}

TEST_F(IdleTimerTest, RepeatNotIdle) {
  
  
  
  

  mock_timer_started = Time::Now();
  TestIdleTask test_task(true);

  TestFinishedTask finish_task;
  ResetIdleTask reset_task;

  base::OneShotTimer<TestFinishedTask> t;
  t.Start(TimeDelta::FromMilliseconds(8 * kSafeTestIntervalMs), &finish_task,
          &TestFinishedTask::Run);

  base::RepeatingTimer<ResetIdleTask> reset_timer;
  reset_timer.Start(TimeDelta::FromMilliseconds(50), &reset_task,
                    &ResetIdleTask::Run);

  test_task.Start();
  MessageLoop::current()->Run();

  reset_timer.Stop();

  EXPECT_EQ(test_task.get_idle_counter(), 0);
}

}  
