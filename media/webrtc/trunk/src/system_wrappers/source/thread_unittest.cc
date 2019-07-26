









#include "system_wrappers/interface/thread_wrapper.h"

#include "gtest/gtest.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

const int kLogTrace = 0;

class TestTraceCallback : public TraceCallback {
 public:
  virtual void Print(const TraceLevel level,
                     const char* traceString,
                     const int length) {
    if (traceString) {
      char* cmd_print = new char[length+1];
      memcpy(cmd_print, traceString, length);
      cmd_print[length] = '\0';
      printf("%s\n", cmd_print);
      fflush(stdout);
      delete[] cmd_print;
    }
  }
};

class ThreadTest : public ::testing::Test {
 public:
  ThreadTest() {
    StartTrace();
  }
  ~ThreadTest() {
    StopTrace();
  }

 private:
  void StartTrace() {
    if (kLogTrace) {
      Trace::CreateTrace();
      Trace::SetLevelFilter(webrtc::kTraceAll);
      Trace::SetTraceCallback(&trace_);
    }
  }

  void StopTrace() {
    if (kLogTrace) {
      Trace::ReturnTrace();
    }
  }

  TestTraceCallback trace_;
};


bool NullRunFunction(void* ) {
  return true;
}

TEST_F(ThreadTest, StartStop) {
  ThreadWrapper* thread = ThreadWrapper::CreateThread(&NullRunFunction);
  unsigned int id = 42;
  ASSERT_TRUE(thread->Start(id));
  EXPECT_TRUE(thread->Stop());
  delete thread;
}


bool SetFlagRunFunction(void* obj) {
  bool* obj_as_bool = static_cast<bool*> (obj);
  *obj_as_bool = true;
  return true;
}

TEST_F(ThreadTest, RunFunctionIsCalled) {
  bool flag = false;
  ThreadWrapper* thread = ThreadWrapper::CreateThread(&SetFlagRunFunction,
                                                      &flag);
  unsigned int id = 42;
  ASSERT_TRUE(thread->Start(id));
  
  EXPECT_TRUE(thread->Stop());
  
  EXPECT_TRUE(flag);
  delete thread;
}

}  
