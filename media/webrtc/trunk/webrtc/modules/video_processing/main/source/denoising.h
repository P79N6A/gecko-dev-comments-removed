












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

    WebRtc_Word32 ChangeUniqueId(WebRtc_Word32 id);

    void Reset();

    WebRtc_Word32 ProcessFrame(I420VideoFrame* frame);

private:
    WebRtc_Word32 _id;

    WebRtc_UWord32*   _moment1;           
    WebRtc_UWord32*   _moment2;           
    WebRtc_UWord32    _frameSize;         
    int               _denoiseFrameCnt;   
};

} 

#endif 
  
