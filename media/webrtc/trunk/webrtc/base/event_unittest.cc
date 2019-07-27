









#include "webrtc/base/event.h"
#include "webrtc/base/gunit.h"

namespace rtc {

TEST(EventTest, InitiallySignaled) {
  Event event(false, true);
  ASSERT_TRUE(event.Wait(0));
}

TEST(EventTest, ManualReset) {
  Event event(true, false);
  ASSERT_FALSE(event.Wait(0));

  event.Set();
  ASSERT_TRUE(event.Wait(0));
  ASSERT_TRUE(event.Wait(0));

  event.Reset();
  ASSERT_FALSE(event.Wait(0));
}

TEST(EventTest, AutoReset) {
  Event event(false, false);
  ASSERT_FALSE(event.Wait(0));

  event.Set();
  ASSERT_TRUE(event.Wait(0));
  ASSERT_FALSE(event.Wait(0));
}

}  
