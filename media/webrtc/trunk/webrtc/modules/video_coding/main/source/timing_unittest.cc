









#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/timing.h"
#include "webrtc/modules/video_coding/main/test/receiver_tests.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

TEST(ReceiverTiming, Tests) {
  SimulatedClock clock(0);
  VCMTiming timing(&clock);
  uint32_t waitTime = 0;
  uint32_t jitterDelayMs = 0;
  uint32_t maxDecodeTimeMs = 0;
  uint32_t timeStamp = 0;

  timing.Reset();

  timing.UpdateCurrentDelay(timeStamp);

  timing.Reset();

  timing.IncomingTimestamp(timeStamp, clock.TimeInMilliseconds());
  jitterDelayMs = 20;
  timing.SetJitterDelay(jitterDelayMs);
  timing.UpdateCurrentDelay(timeStamp);
  timing.set_render_delay(0);
  waitTime = timing.MaxWaitingTime(
      timing.RenderTimeMs(timeStamp, clock.TimeInMilliseconds()),
      clock.TimeInMilliseconds());
  
  
  EXPECT_EQ(jitterDelayMs, waitTime);

  jitterDelayMs += VCMTiming::kDelayMaxChangeMsPerS + 10;
  timeStamp += 90000;
  clock.AdvanceTimeMilliseconds(1000);
  timing.SetJitterDelay(jitterDelayMs);
  timing.UpdateCurrentDelay(timeStamp);
  waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(
      timeStamp, clock.TimeInMilliseconds()), clock.TimeInMilliseconds());
  
  EXPECT_EQ(jitterDelayMs - 10, waitTime);

  timeStamp += 90000;
  clock.AdvanceTimeMilliseconds(1000);
  timing.UpdateCurrentDelay(timeStamp);
  waitTime = timing.MaxWaitingTime(
      timing.RenderTimeMs(timeStamp, clock.TimeInMilliseconds()),
      clock.TimeInMilliseconds());
  EXPECT_EQ(waitTime, jitterDelayMs);

  
  
  for (int i = 0; i < 300; i++) {
    clock.AdvanceTimeMilliseconds(1000 / 25);
    timeStamp += 90000 / 25;
    timing.IncomingTimestamp(timeStamp, clock.TimeInMilliseconds());
  }
  timing.UpdateCurrentDelay(timeStamp);
  waitTime = timing.MaxWaitingTime(
      timing.RenderTimeMs(timeStamp, clock.TimeInMilliseconds()),
      clock.TimeInMilliseconds());
  EXPECT_EQ(waitTime, jitterDelayMs);

  
  for (int i = 0; i < 10; i++) {
    int64_t startTimeMs = clock.TimeInMilliseconds();
    clock.AdvanceTimeMilliseconds(10);
    timing.StopDecodeTimer(timeStamp, startTimeMs,
                           clock.TimeInMilliseconds());
    timeStamp += 90000 / 25;
    clock.AdvanceTimeMilliseconds(1000 / 25 - 10);
    timing.IncomingTimestamp(timeStamp, clock.TimeInMilliseconds());
  }
  maxDecodeTimeMs = 10;
  timing.SetJitterDelay(jitterDelayMs);
  clock.AdvanceTimeMilliseconds(1000);
  timeStamp += 90000;
  timing.UpdateCurrentDelay(timeStamp);
  waitTime = timing.MaxWaitingTime(
      timing.RenderTimeMs(timeStamp, clock.TimeInMilliseconds()),
      clock.TimeInMilliseconds());
  EXPECT_EQ(waitTime, jitterDelayMs);

  uint32_t minTotalDelayMs = 200;
  timing.set_min_playout_delay(minTotalDelayMs);
  clock.AdvanceTimeMilliseconds(5000);
  timeStamp += 5*90000;
  timing.UpdateCurrentDelay(timeStamp);
  const int kRenderDelayMs = 10;
  timing.set_render_delay(kRenderDelayMs);
  waitTime = timing.MaxWaitingTime(
      timing.RenderTimeMs(timeStamp, clock.TimeInMilliseconds()),
      clock.TimeInMilliseconds());
  
  
  EXPECT_EQ(waitTime, minTotalDelayMs - maxDecodeTimeMs - kRenderDelayMs);
  
  EXPECT_EQ(minTotalDelayMs, timing.TargetVideoDelay());

  
  timing.set_min_playout_delay(0);
  clock.AdvanceTimeMilliseconds(5000);
  timeStamp += 5*90000;
  timing.UpdateCurrentDelay(timeStamp);
}

TEST(ReceiverTiming, WrapAround) {
  const int kFramerate = 25;
  SimulatedClock clock(0);
  VCMTiming timing(&clock);
  
  uint32_t timestamp = 0xFFFFFFFFu - 3 * 90000 / kFramerate;
  for (int i = 0; i < 4; ++i) {
    timing.IncomingTimestamp(timestamp, clock.TimeInMilliseconds());
    clock.AdvanceTimeMilliseconds(1000 / kFramerate);
    timestamp += 90000 / kFramerate;
    int64_t render_time = timing.RenderTimeMs(0xFFFFFFFFu,
                                              clock.TimeInMilliseconds());
    EXPECT_EQ(3 * 1000 / kFramerate, render_time);
    render_time = timing.RenderTimeMs(89u,  
                                      clock.TimeInMilliseconds());
    EXPECT_EQ(3 * 1000 / kFramerate + 1, render_time);
  }
}

}  
