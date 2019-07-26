












#ifndef VPM_DENOISING_H
#define VPM_DENOISING_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMDenoising
{
public:
    VPMDenoising();
    ~VPMDenoising();

    int32_t ChangeUniqueId(int32_t id);

    void Reset();

    int32_t ProcessFrame(I420VideoFrame* frame);

private:
    int32_t _id;

    uint32_t*   _moment1;           
    uint32_t*   _moment2;           
    uint32_t    _frameSize;         
    int               _denoiseFrameCnt;   
};

}  

#endif 
  
