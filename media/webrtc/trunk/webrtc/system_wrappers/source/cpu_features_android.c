









#include "webrtc/system_wrappers/source/droid-cpu-features.h"

uint64_t WebRtc_GetCPUFeaturesARM(void) {
  return android_getCpuFeatures();
}
