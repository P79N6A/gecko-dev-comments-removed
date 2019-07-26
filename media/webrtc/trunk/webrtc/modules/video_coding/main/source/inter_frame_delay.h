









#ifndef WEBRTC_MODULES_VIDEO_CODING_INTER_FRAME_DELAY_H_
#define WEBRTC_MODULES_VIDEO_CODING_INTER_FRAME_DELAY_H_

#include "webrtc/typedefs.h"

namespace webrtc
{

class VCMInterFrameDelay
{
public:
    VCMInterFrameDelay(int64_t currentWallClock);

    
    void Reset(int64_t currentWallClock);

    
    
    
    
    
    
    
    
    
    bool CalculateDelay(uint32_t timestamp,
                        int64_t *delay,
                        int64_t currentWallClock);

    
    
    
    
    uint32_t CurrentTimeStampDiffMs() const;

private:
    
    
    
    
    
    void CheckForWrapArounds(uint32_t timestamp);

    int64_t         _zeroWallClock; 
    int32_t         _wrapArounds;   
    
    uint32_t        _prevTimestamp;
    
    int64_t         _prevWallClock;
    
    int64_t         _dTS;
};

}  

#endif 
