









#ifndef MODULES_VIDEO_PROCESSING_MAIN_SOURCE_BRIGHTEN_H_
#define MODULES_VIDEO_PROCESSING_MAIN_SOURCE_BRIGHTEN_H_

#include "typedefs.h"
#include "modules/video_processing/main/interface/video_processing.h"

namespace webrtc {
namespace VideoProcessing {

WebRtc_Word32 Brighten(I420VideoFrame* frame, int delta);

}  
}  

#endif  
