



#include <windows.h>
#include <mmsystem.h>
#include <process.h>

#include "base/time.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace {

class MockTimeTicks : public TimeTicks {
 public:
  static DWORD Ticker() {
    return static_cast<int>(InterlockedIncrement(&ticker_));
  }

  static void InstallTicker() {
    old_tick_function_ = SetMockTickFunction(&Ticker);
    ticker_ = -5;
  }

  static void UninstallTicker() {
    SetMockTickFunction(old_tick_function_);
  }

 private:
  static volatile LONG ticker_;
  static TickFunctionType old_tick_function_;
};

volatile LONG MockTimeTicks::ticker_;
MockTimeTicks::TickFunctionType MockTimeTicks::old_tick_function_;

HANDLE g_rollover_test_start;

unsigned __stdcall RolloverTestThreadMain(void* param) {
  int64 counter = reinterpret_cast<int64>(param);
  DWORD rv = WaitForSingleObject(g_rollover_test_start, INFINITE);
  EXPECT_EQ(rv, WAIT_OBJECT_0);

  TimeTicks last = TimeTicks::Now();
  for (int index = 0; index < counter; index++) {
    TimeTicks now = TimeTicks::Now();
    int64 milliseconds = (now - last).InMilliseconds();
    
    
    EXPECT_GE(milliseconds, 0);
    EXPECT_LT(milliseconds, 250);
    last = now;
  }
  return 0;
}

}  

TEST(TimeTicks, WinRollover) {
  
  
  
  
  
  
  
  

  const int kThreads = 8;
  
  const int64 kChecks = 10;

  
  
  for (int loop = 0; loop < 4096; loop++) {
    
    MockTimeTicks::InstallTicker();
    g_rollover_test_start = CreateEvent(0, TRUE, FALSE, 0);
    HANDLE threads[kThreads];

    for (int index = 0; index < kThreads; index++) {
      void* argument = reinterpret_cast<void*>(kChecks);
      unsigned thread_id;
      threads[index] = reinterpret_cast<HANDLE>(
        _beginthreadex(NULL, 0, RolloverTestThreadMain, argument, 0,
          &thread_id));
      EXPECT_NE((HANDLE)NULL, threads[index]);
    }

    
    SetEvent(g_rollover_test_start);

    
    for (int index = 0; index < kThreads; index++) {
      DWORD rv = WaitForSingleObject(threads[index], INFINITE);
      EXPECT_EQ(rv, WAIT_OBJECT_0);
    }

    CloseHandle(g_rollover_test_start);

    
    MockTimeTicks::UninstallTicker();
  }
}

TEST(TimeTicks, SubMillisecondTimers) {
  
  
  
  bool saw_submillisecond_timer = false;
  int64 min_timer = 1000;
  TimeTicks last_time = TimeTicks::HighResNow();
  for (int index = 0; index < 1000; index++) {
    TimeTicks now = TimeTicks::HighResNow();
    TimeDelta delta = now - last_time;
    if (delta.InMicroseconds() > 0 &&
        delta.InMicroseconds() < 1000) {
      if (min_timer > delta.InMicroseconds())
        min_timer = delta.InMicroseconds();
      saw_submillisecond_timer = true;
    }
    last_time = now;
  }
  EXPECT_TRUE(saw_submillisecond_timer);
  printf("Min timer is: %ldus\n", static_cast<long>(min_timer));
}

TEST(TimeTicks, TimeGetTimeCaps) {
  

  TIMECAPS caps;
  MMRESULT status = timeGetDevCaps(&caps, sizeof(caps));
  EXPECT_EQ(TIMERR_NOERROR, status);
  if (status != TIMERR_NOERROR) {
    printf("Could not get timeGetDevCaps\n");
    return;
  }

  EXPECT_GE(static_cast<int>(caps.wPeriodMin), 1);
  EXPECT_GT(static_cast<int>(caps.wPeriodMax), 1);
  EXPECT_GE(static_cast<int>(caps.wPeriodMin), 1);
  EXPECT_GT(static_cast<int>(caps.wPeriodMax), 1);
  printf("timeGetTime range is %d to %dms\n", caps.wPeriodMin,
    caps.wPeriodMax);
}

TEST(TimeTicks, QueryPerformanceFrequency) {
  

  LARGE_INTEGER frequency;
  BOOL rv = QueryPerformanceFrequency(&frequency);
  EXPECT_EQ(TRUE, rv);
  EXPECT_GT(frequency.QuadPart, 1000000);  
  printf("QueryPerformanceFrequency is %5.2fMHz\n",
    frequency.QuadPart / 1000000.0);
}

TEST(TimeTicks, TimerPerformance) {
  
  
  const int kLoops = 10000;
  
  
  
  const int kMaxTime = 35;  

  typedef TimeTicks (*TestFunc)();
  struct TestCase {
    TestFunc func;
    char *description;
  };
  
  
  COMPILE_ASSERT(sizeof(TimeTicks) == sizeof(Time),
                 test_only_works_with_same_sizes);
  TestCase cases[] = {
    { reinterpret_cast<TestFunc>(Time::Now), "Time::Now" },
    { TimeTicks::Now, "TimeTicks::Now" },
    { TimeTicks::HighResNow, "TimeTicks::HighResNow" },
    { NULL, "" }
  };

  int test_case = 0;
  while (cases[test_case].func) {
    TimeTicks start = TimeTicks::HighResNow();
    for (int index = 0; index < kLoops; index++)
      cases[test_case].func();
    TimeTicks stop = TimeTicks::HighResNow();
    
    
    
    
    
    
    
    printf("%s: %1.2fus per call\n", cases[test_case].description,
      (stop - start).InMillisecondsF() * 1000 / kLoops);
    test_case++;
  }
}
