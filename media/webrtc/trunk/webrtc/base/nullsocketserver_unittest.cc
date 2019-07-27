









#include "webrtc/base/gunit.h"
#include "webrtc/base/nullsocketserver.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

static const uint32 kTimeout = 5000U;

class NullSocketServerTest
    : public testing::Test,
      public MessageHandler {
 public:
  NullSocketServerTest() {}
 protected:
  virtual void OnMessage(Message* message) {
    ss_.WakeUp();
  }
  NullSocketServer ss_;
};

TEST_F(NullSocketServerTest, WaitAndSet) {
  Thread thread;
  EXPECT_TRUE(thread.Start());
  thread.Post(this, 0);
  
  const bool process_io = true;
  EXPECT_TRUE_WAIT(ss_.Wait(rtc::kForever, process_io), kTimeout);
}

TEST_F(NullSocketServerTest, TestWait) {
  uint32 start = Time();
  ss_.Wait(200, true);
  
  
  EXPECT_GE(TimeSince(start), 180);
}

}  
