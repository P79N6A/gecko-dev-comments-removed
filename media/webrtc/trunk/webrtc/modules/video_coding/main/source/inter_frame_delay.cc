









#include "webrtc/modules/video_coding/main/source/inter_frame_delay.h"

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
VCMInterFrameDelay::CalculateDelay(uint32_t timestamp,
                                int64_t *delay,
                                int64_t currentWallClock)
{
    if (_prevWallClock == 0)
    {
        
        _prevWallClock = currentWallClock;
        _prevTimestamp = timestamp;
        *delay = 0;
        return true;
    }

    int32_t prevWrapArounds = _wrapArounds;
    CheckForWrapArounds(timestamp);

    
    int32_t wrapAroundsSincePrev = _wrapArounds - prevWrapArounds;

    
    
    
    
    if ((wrapAroundsSincePrev == 0 && timestamp < _prevTimestamp) || wrapAroundsSincePrev < 0)
    {
        *delay = 0;
        return false;
    }

    
    
    _dTS = static_cast<int64_t>((timestamp + wrapAroundsSincePrev *
                (static_cast<int64_t>(1)<<32) - _prevTimestamp) / 90.0 + 0.5);

    
    
    
    *delay = static_cast<int64_t>(currentWallClock - _prevWallClock - _dTS);

    _prevTimestamp = timestamp;
    _prevWallClock = currentWallClock;

    return true;
}


uint32_t VCMInterFrameDelay::CurrentTimeStampDiffMs() const
{
    if (_dTS < 0)
    {
        return 0;
    }
    return static_cast<uint32_t>(_dTS);
}



void
VCMInterFrameDelay::CheckForWrapArounds(uint32_t timestamp)
{
    if (timestamp < _prevTimestamp)
    {
        
        
        
        if (static_cast<int32_t>(timestamp - _prevTimestamp) > 0)
        {
            
            _wrapArounds++;
        }
    }
    
    
    else if (static_cast<int32_t>(_prevTimestamp - timestamp) > 0)
    {
        
        _wrapArounds--;
    }
}

}
