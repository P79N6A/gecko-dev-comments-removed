











#include "webrtc/modules/audio_processing/aec/include/echo_cancellation.h"

#include <stdlib.h>
#include <time.h>

extern "C" {
#include "webrtc/modules/audio_processing/aec/aec_core.h"
}

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {

TEST(EchoCancellationTest, CreateAndFreeHandlesErrors) {
  EXPECT_EQ(-1, WebRtcAec_Create(NULL));
  void* handle = NULL;
  ASSERT_EQ(0, WebRtcAec_Create(&handle));
  EXPECT_TRUE(handle != NULL);
  EXPECT_EQ(-1, WebRtcAec_Free(NULL));
  EXPECT_EQ(0, WebRtcAec_Free(handle));
}

TEST(EchoCancellationTest, ApplyAecCoreHandle) {
  void* handle = NULL;
  ASSERT_EQ(0, WebRtcAec_Create(&handle));
  EXPECT_TRUE(handle != NULL);
  EXPECT_TRUE(WebRtcAec_aec_core(NULL) == NULL);
  AecCore* aec_core = WebRtcAec_aec_core(handle);
  EXPECT_TRUE(aec_core != NULL);
  
  
  int delay = 111;
  WebRtcAec_SetSystemDelay(aec_core, delay);
  EXPECT_EQ(delay, WebRtcAec_system_delay(aec_core));
  EXPECT_EQ(0, WebRtcAec_Free(handle));
}

}  
