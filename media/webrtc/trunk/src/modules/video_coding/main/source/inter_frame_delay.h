









#ifndef WEBRTC_MODULES_VIDEO_CODING_INTER_FRAME_DELAY_H_
#define WEBRTC_MODULES_VIDEO_CODING_INTER_FRAME_DELAY_H_

#include "typedefs.h"

namespace webrtc
{

class VCMInterFrameDelay
{
public:
    VCMInterFrameDelay(int64_t currentWallClock);

    
    void Reset(int64_t currentWallClock);

    
    
    
    
    
    
    
    
    
    bool CalculateDelay(WebRtc_UWord32 timestamp,
                        WebRtc_Word64 *delay,
                        int64_t currentWallClock);

    
    
    
    
    WebRtc_UWord32 CurrentTimeStampDiffMs() const;

private:
    
    
    
    
    
    void CheckForWrapArounds(WebRtc_UWord32 timestamp);

    WebRtc_Word64         _zeroWallClock; 
    WebRtc_Word32         _wrapArounds;   
    
    WebRtc_UWord32        _prevTimestamp;
    
    WebRtc_Word64         _prevWallClock;
    
    WebRtc_Word64         _dTS;
};

} 

#endif 
