









#include "webrtc/modules/desktop_capture/differ_block.h"

#include <string.h>

#include "build/build_config.h"
#include "webrtc/modules/desktop_capture/differ_block_sse2.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"

namespace webrtc {

int BlockDifference_C(const uint8_t* image1,
                      const uint8_t* image2,
                      int stride) {
  int width_bytes = kBlockSize * kBytesPerPixel;

  for (int y = 0; y < kBlockSize; y++) {
    if (memcmp(image1, image2, width_bytes) != 0)
      return 1;
    image1 += stride;
    image2 += stride;
  }
  return 0;
}

int BlockDifference(const uint8_t* image1, const uint8_t* image2, int stride) {
  static int (*diff_proc)(const uint8_t*, const uint8_t*, int) = NULL;

  if (!diff_proc) {
#if !defined(WEBRTC_ARCH_X86_FAMILY)
    
    
    diff_proc = &BlockDifference_C;
#else
    bool have_sse2 = WebRtc_GetCPUInfo(kSSE2) != 0;
    
    if (have_sse2 && kBlockSize == 32) {
      diff_proc = &BlockDifference_SSE2_W32;
    } else if (have_sse2 && kBlockSize == 16) {
      diff_proc = &BlockDifference_SSE2_W16;
    } else {
      diff_proc = &BlockDifference_C;
    }
#endif
  }

  return diff_proc(image1, image2, stride);
}

}  
