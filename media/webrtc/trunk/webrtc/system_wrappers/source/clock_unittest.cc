









#include "webrtc/system_wrappers/interface/clock.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {

TEST(ClockTest, NtpTime) {
  Clock* clock = Clock::GetRealTimeClock();
  uint32_t seconds;
  uint32_t fractions;
  clock->CurrentNtp(seconds, fractions);
  int64_t milliseconds = clock->CurrentNtpInMilliseconds();
  EXPECT_GE(milliseconds, Clock::NtpToMs(seconds, fractions));
  EXPECT_NEAR(milliseconds, Clock::NtpToMs(seconds, fractions), 5);
}
}  
