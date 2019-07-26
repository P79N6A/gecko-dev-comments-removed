









#include <stdlib.h>  

#include "webrtc/modules/video_processing/main/source/color_enhancement.h"
#include "webrtc/modules/video_processing/main/source/color_enhancement_private.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
namespace VideoProcessing {

int32_t ColorEnhancement(I420VideoFrame* frame) {
assert(frame);

uint8_t* ptr_u;
uint8_t* ptr_v;
uint8_t temp_chroma;
if (frame->IsZeroSize()) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing,
               -1, "Null frame pointer");
  return VPM_GENERAL_ERROR;
}

if (frame->width() == 0 || frame->height() == 0) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing,
               -1, "Invalid frame size");
  return VPM_GENERAL_ERROR;
}


ptr_u = frame->buffer(kUPlane);
ptr_v = frame->buffer(kVPlane);
int size_uv = ((frame->width() + 1) / 2) * ((frame->height() + 1) / 2);


for (int ix = 0; ix < size_uv; ix++) {
  temp_chroma = colorTable[*ptr_u][*ptr_v];
  *ptr_v = colorTable[*ptr_v][*ptr_u];
  *ptr_u = temp_chroma;

  ptr_u++;
  ptr_v++;
}
return VPM_OK;
}

}  
}  
