









#if defined(WEBRTC_CHROMIUM_BUILD)
#include <cpu-features.h>
#else
#include "webrtc/system_wrappers/source/android/cpu-features.h"
#endif  

uint64_t WebRtc_GetCPUFeaturesARM(void) {
  return android_getCpuFeatures();
}
