









#include "webrtc/system_wrappers/interface/thread_wrapper.h"

#include "gtest/gtest.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

TEST(ThreadTest, NullFunctionPointer) {
  webrtc::scoped_ptr<ThreadWrapper> thread(
    webrtc::ThreadWrapper::CreateThread());
  unsigned int id = 42;
  EXPECT_FALSE(thread->Start(id));
}


bool NullRunFunction(void* obj) {
  return true;
}

TEST(ThreadTest, StartStop) {
  ThreadWrapper* thread = ThreadWrapper::CreateThread(&NullRunFunction);
  unsigned int id = 42;
  ASSERT_TRUE(thread->Start(id));
  EXPECT_TRUE(thread->Stop());
  delete thread;
}


bool SetFlagRunFunction(void* obj) {
  bool* obj_as_bool = static_cast<bool*>(obj);
  *obj_as_bool = true;
  return true;
}

TEST(ThreadTest, RunFunctionIsCalled) {
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
