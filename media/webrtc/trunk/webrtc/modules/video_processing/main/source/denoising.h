












#ifndef VPM_DENOISING_H
#define VPM_DENOISING_H

#include "typedefs.h"
#include "video_processing.h"

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
  
