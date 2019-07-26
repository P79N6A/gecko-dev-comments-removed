









#include "webrtc/system_wrappers/source/thread_posix.h"

#include "gtest/gtest.h"

TEST(ThreadTestPosix, PrioritySettings) {
  
  const int kMinPrio = -1;
  const int kMaxPrio = 2;

  int last_priority = kMinPrio;
  for (int priority = webrtc::kLowPriority;
       priority <= webrtc::kRealtimePriority; ++priority) {
    int system_priority = webrtc::ConvertToSystemPriority(
        static_cast<webrtc::ThreadPriority>(priority), kMinPrio, kMaxPrio);
    EXPECT_GT(system_priority, kMinPrio);
    EXPECT_LT(system_priority, kMaxPrio);
    EXPECT_GE(system_priority, last_priority);
    last_priority = system_priority;
  }
}
