












#ifndef VPM_BRIGHTNESS_DETECTION_H
#define VPM_BRIGHTNESS_DETECTION_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMBrightnessDetection
{
public:
    VPMBrightnessDetection();
    ~VPMBrightnessDetection();

    int32_t ChangeUniqueId(int32_t id);

    void Reset();

    int32_t ProcessFrame(const I420VideoFrame& frame,
                         const VideoProcessingModule::FrameStats& stats);

private:
    int32_t _id;

    uint32_t _frameCntBright;
    uint32_t _frameCntDark;
};

}  

#endif 
