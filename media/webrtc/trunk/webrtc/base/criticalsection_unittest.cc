









#include <set>
#include <vector>

#include "webrtc/base/criticalsection.h"
#include "webrtc/base/event.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/scopedptrcollection.h"
#include "webrtc/base/thread.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

namespace {

const int kLongTime = 10000;  
const int kNumThreads = 16;
const int kOperationsToRun = 1000;

template <class T>
class AtomicOpRunner : public MessageHandler {
 public:
  explicit AtomicOpRunner(int initial_value)
      : value_(initial_value),
        threads_active_(0),
        start_event_(true, false),
        done_event_(true, false) {}

  int value() const { return value_; }

  bool Run() {
    
    start_event_.Set();

    
    return done_event_.Wait(kLongTime);
  }

  void SetExpectedThreadCount(int count) {
    threads_active_ = count;
  }

  virtual void OnMessage(Message* msg) {
    std::vector<int> values;
    values.reserve(kOperationsToRun);

    
    ASSERT_TRUE(start_event_.Wait(kLongTime));

    
    for (int i = 0; i < kOperationsToRun; ++i) {
      values.push_back(T::AtomicOp(&value_));
    }

    { 
      CritScope cs(&all_values_crit_);
      for (size_t i = 0; i < values.size(); ++i) {
        std::pair<std::set<int>::iterator, bool> result =
            all_values_.insert(values[i]);
        
        
        EXPECT_TRUE(result.second)
            << "Thread=" << Thread::Current() << " value=" << values[i];
      }
    }

    
    if (AtomicOps::Decrement(&threads_active_) == 0) {
      done_event_.Set();
    }
  }

 private:
  int value_;
  int threads_active_;
  CriticalSection all_values_crit_;
  std::set<int> all_values_;
  Event start_event_;
  Event done_event_;
};

struct IncrementOp {
  static int AtomicOp(int* i) { return AtomicOps::Increment(i); }
};

struct DecrementOp {
  static int AtomicOp(int* i) { return AtomicOps::Decrement(i); }
};

void StartThreads(ScopedPtrCollection<Thread>* threads,
                  MessageHandler* handler) {
  for (int i = 0; i < kNumThreads; ++i) {
    Thread* thread = new Thread();
    thread->Start();
    thread->Post(handler);
    threads->PushBack(thread);
  }
}

}  

TEST(AtomicOpsTest, Simple) {
  int value = 0;
  EXPECT_EQ(1, AtomicOps::Increment(&value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(2, AtomicOps::Increment(&value));
  EXPECT_EQ(2, value);
  EXPECT_EQ(1, AtomicOps::Decrement(&value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(0, AtomicOps::Decrement(&value));
  EXPECT_EQ(0, value);
}

TEST(AtomicOpsTest, Increment) {
  
  AtomicOpRunner<IncrementOp> runner(0);
  ScopedPtrCollection<Thread> threads;
  StartThreads(&threads, &runner);
  runner.SetExpectedThreadCount(kNumThreads);

  
  EXPECT_TRUE(runner.Run());
  EXPECT_EQ(kOperationsToRun * kNumThreads, runner.value());
}

TEST(AtomicOpsTest, Decrement) {
  
  AtomicOpRunner<DecrementOp> runner(kOperationsToRun * kNumThreads);
  ScopedPtrCollection<Thread> threads;
  StartThreads(&threads, &runner);
  runner.SetExpectedThreadCount(kNumThreads);

  
  EXPECT_TRUE(runner.Run());
  EXPECT_EQ(0, runner.value());
}

}  
