












#ifndef VPM_COLOR_ENHANCEMENT_H
#define VPM_COLOR_ENHANCEMENT_H

#include "typedefs.h"
#include "video_processing.h"

namespace webrtc {

namespace VideoProcessing
{
    WebRtc_Word32 ColorEnhancement(WebRtc_UWord8* frame,
                                 WebRtc_UWord32 width,
                                 WebRtc_UWord32 height);
}

} 

#endif 
