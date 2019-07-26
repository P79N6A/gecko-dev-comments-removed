









#include "webrtc/system_wrappers/interface/tick_util.h"

#include <cassert>

namespace webrtc {

bool TickTime::use_fake_clock_ = false;
int64_t TickTime::fake_ticks_ = 0;

void TickTime::UseFakeClock(int64_t start_millisecond) {
  use_fake_clock_ = true;
  fake_ticks_ = MillisecondsToTicks(start_millisecond);
}

void TickTime::AdvanceFakeClock(int64_t milliseconds) {
  assert(use_fake_clock_);
  fake_ticks_ += MillisecondsToTicks(milliseconds);
}

}  
