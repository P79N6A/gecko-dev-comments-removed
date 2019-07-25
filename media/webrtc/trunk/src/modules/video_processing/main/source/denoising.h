












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

    WebRtc_Word32 ProcessFrame(WebRtc_UWord8* frame,
                             WebRtc_UWord32 width,
                             WebRtc_UWord32 height);

private:
    WebRtc_Word32 _id;

    WebRtc_UWord32*   _moment1;           
    WebRtc_UWord32*   _moment2;           
    WebRtc_UWord32    _frameSize;         
    WebRtc_Word32     _denoiseFrameCnt;   
};

} 

#endif 
  
