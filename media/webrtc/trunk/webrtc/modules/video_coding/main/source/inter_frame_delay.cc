









#include "inter_frame_delay.h"

namespace webrtc {

VCMInterFrameDelay::VCMInterFrameDelay(int64_t currentWallClock)
{
    Reset(currentWallClock);
}


void
VCMInterFrameDelay::Reset(int64_t currentWallClock)
{
    _zeroWallClock = currentWallClock;
    _wrapArounds = 0;
    _prevWallClock = 0;
    _prevTimestamp = 0;
    _dTS = 0;
}



bool
VCMInterFrameDelay::CalculateDelay(WebRtc_UWord32 timestamp,
                                WebRtc_Word64 *delay,
                                int64_t currentWallClock)
{
    if (_prevWallClock == 0)
    {
        
        _prevWallClock = currentWallClock;
        _prevTimestamp = timestamp;
        *delay = 0;
        return true;
    }

    WebRtc_Word32 prevWrapArounds = _wrapArounds;
    CheckForWrapArounds(timestamp);

    
    WebRtc_Word32 wrapAroundsSincePrev = _wrapArounds - prevWrapArounds;

    
    
    
    
    if ((wrapAroundsSincePrev == 0 && timestamp < _prevTimestamp) || wrapAroundsSincePrev < 0)
    {
        *delay = 0;
        return false;
    }

    
    
    _dTS = static_cast<WebRtc_Word64>((timestamp + wrapAroundsSincePrev *
                (static_cast<WebRtc_Word64>(1)<<32) - _prevTimestamp) / 90.0 + 0.5);

    
    
    
    *delay = static_cast<WebRtc_Word64>(currentWallClock - _prevWallClock - _dTS);

    _prevTimestamp = timestamp;
    _prevWallClock = currentWallClock;

    return true;
}


WebRtc_UWord32 VCMInterFrameDelay::CurrentTimeStampDiffMs() const
{
    if (_dTS < 0)
    {
        return 0;
    }
    return static_cast<WebRtc_UWord32>(_dTS);
}



void
VCMInterFrameDelay::CheckForWrapArounds(WebRtc_UWord32 timestamp)
{
    if (timestamp < _prevTimestamp)
    {
        
        
        
        if (static_cast<WebRtc_Word32>(timestamp - _prevTimestamp) > 0)
        {
            
            _wrapArounds++;
        }
    }
    
    
    else if (static_cast<WebRtc_Word32>(_prevTimestamp - timestamp) > 0)
    {
        
        _wrapArounds--;
    }
}

}
