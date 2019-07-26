









#include "webrtc/system_wrappers/interface/tick_util.h"

#include <cassert>

namespace webrtc {

bool TickTime::use_fake_clock_ = false;
WebRtc_Word64 TickTime::fake_ticks_ = 0;

void TickTime::UseFakeClock(WebRtc_Word64 start_millisecond) {
  use_fake_clock_ = true;
  fake_ticks_ = MillisecondsToTicks(start_millisecond);
}

void TickTime::AdvanceFakeClock(WebRtc_Word64 milliseconds) {
  assert(use_fake_clock_);
  fake_ticks_ += MillisecondsToTicks(milliseconds);
}

}  
