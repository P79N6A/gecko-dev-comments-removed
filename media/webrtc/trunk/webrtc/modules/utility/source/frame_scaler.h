











#ifndef WEBRTC_MODULES_UTILITY_SOURCE_FRAME_SCALER_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_FRAME_SCALER_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "common_video/interface/i420_video_frame.h"
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

    
    
    int ResizeFrameIfNeeded(I420VideoFrame* video_frame,
                            int out_width,
                            int out_height);

 private:
    scoped_ptr<Scaler> scaler_;
    I420VideoFrame scaled_frame_;
};

}  

#endif  

#endif  
