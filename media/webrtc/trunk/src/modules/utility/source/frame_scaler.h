











#ifndef WEBRTC_MODULES_UTILITY_SOURCE_FRAME_SCALER_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_FRAME_SCALER_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "engine_configurations.h"
#include "modules/interface/module_common_types.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Scaler;
class VideoFrame;

class FrameScaler {
 public:
    FrameScaler();
    ~FrameScaler();

    
    
    int ResizeFrameIfNeeded(VideoFrame* video_frame,
                            WebRtc_UWord32 out_width,
                            WebRtc_UWord32 out_height);

 private:
    scoped_ptr<Scaler> scaler_;
    VideoFrame scaled_frame_;
};

}  

#endif  

#endif  
