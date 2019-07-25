












#ifndef VPM_BRIGHTNESS_DETECTION_H
#define VPM_BRIGHTNESS_DETECTION_H

#include "typedefs.h"
#include "video_processing.h"

namespace webrtc {

class VPMBrightnessDetection
{
public:
    VPMBrightnessDetection();
    ~VPMBrightnessDetection();

    WebRtc_Word32 ChangeUniqueId(WebRtc_Word32 id);

    void Reset();

    WebRtc_Word32 ProcessFrame(const WebRtc_UWord8* frame,
                             WebRtc_UWord32 width,
                             WebRtc_UWord32 height,
                             const VideoProcessingModule::FrameStats& stats);

private:
    WebRtc_Word32 _id;

    WebRtc_UWord32 _frameCntBright;
    WebRtc_UWord32 _frameCntDark;
};

} 

#endif 
