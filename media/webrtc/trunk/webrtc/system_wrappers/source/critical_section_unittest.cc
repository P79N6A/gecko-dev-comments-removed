









#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/source/unittest_utilities.h"

namespace webrtc {

namespace {

const bool kLogTrace = false;  



static void SwitchProcess() {
  
  
  SleepMs(1);
}

class ProtectedCount {
public:
  explicit ProtectedCount(CriticalSectionWrapper* crit_sect)
    : crit_sect_(crit_sect),
      count_(0) {
  }

  void Increment() {
    CriticalSectionScoped cs(crit_sect_);
    ++count_;
  }

  int Count() const {
    CriticalSectionScoped cs(crit_sect_);
    return count_;
  }

private:
  CriticalSectionWrapper* crit_sect_;
  int count_;
};

class CritSectTest : public ::testing::Test {
public:
  CritSectTest() : trace_(kLogTrace) {
  }

  
  
  bool WaitForCount(int target, ProtectedCount* count) {
    int loop_counter = 0;
    
    
    
    while (count->Count() < target && loop_counter < 100 * target) {
      ++loop_counter;
      SwitchProcess();
    }
    return (count->Count() >= target);
  }

private:
  ScopedTracing trace_;
};

bool LockUnlockThenStopRunFunction(void* obj) {
  ProtectedCount* the_count = static_cast<ProtectedCount*>(obj);
  the_count->Increment();
  return false;
}

TEST_F(CritSectTest, ThreadWakesOnce) NO_THREAD_SAFETY_ANALYSIS {
  CriticalSectionWrapper* crit_sect =
      CriticalSectionWrapper::CreateCriticalSection();
  ProtectedCount count(crit_sect);
  ThreadWrapper* thread = ThreadWrapper::CreateThread(
      &LockUnlockThenStopRunFunction, &count);
  unsigned int id = 42;
  crit_sect->Enter();
  ASSERT_TRUE(thread->Start(id));
  SwitchProcess();
  
  
  
  
  ASSERT_EQ(0, count.Count());
  crit_sect->Leave();  
  EXPECT_TRUE(WaitForCount(1, &count));
  EXPECT_TRUE(thread->Stop());
  delete thread;
  delete crit_sect;
}

bool LockUnlockRunFunction(void* obj) {
  ProtectedCount* the_count = static_cast<ProtectedCount*>(obj);
  the_count->Increment();
  SwitchProcess();
  return true;
}

TEST_F(CritSectTest, ThreadWakesTwice) NO_THREAD_SAFETY_ANALYSIS {
  CriticalSectionWrapper* crit_sect =
      CriticalSectionWrapper::CreateCriticalSection();
  ProtectedCount count(crit_sect);
  ThreadWrapper* thread = ThreadWrapper::CreateThread(&LockUnlockRunFunction,
                                                      &count);
  unsigned int id = 42;
  crit_sect->Enter();  
  ASSERT_TRUE(thread->Start(id));
  crit_sect->Leave();

  
  
  
  EXPECT_TRUE(WaitForCount(2, &count));
  EXPECT_LE(2, count.Count());

  
  crit_sect->Enter();
  int count_before = count.Count();
  for (int i = 0; i < 10; i++) {
    SwitchProcess();
  }
  EXPECT_EQ(count_before, count.Count());
  crit_sect->Leave();

  thread->SetNotAlive();  
  SwitchProcess();
  EXPECT_TRUE(WaitForCount(count_before + 1, &count));
  EXPECT_TRUE(thread->Stop());
  delete thread;
  delete crit_sect;
}

}  

}  
