












#ifndef VPM_VIDEO_DECIMATOR_H
#define VPM_VIDEO_DECIMATOR_H

#include "typedefs.h"
#include "module_common_types.h"

namespace webrtc {

class VPMVideoDecimator
{
public:
    VPMVideoDecimator();
    ~VPMVideoDecimator();
    
    void Reset();
    
    void EnableTemporalDecimation(bool enable);
    
    WebRtc_Word32 SetMaxFrameRate(WebRtc_UWord32 maxFrameRate);
    WebRtc_Word32 SetTargetFrameRate(WebRtc_UWord32 frameRate);

    bool DropFrame();
    
    void UpdateIncomingFrameRate();

    
    WebRtc_UWord32 DecimatedFrameRate();

    
    WebRtc_UWord32 InputFrameRate();

private:
    void ProcessIncomingFrameRate(WebRtc_Word64 now);

    enum { kFrameCountHistorySize = 90};
    enum { kFrameHistoryWindowMs = 2000};

    
    WebRtc_Word32         _overShootModifier;
    WebRtc_UWord32        _dropCount;
    WebRtc_UWord32        _keepCount;
    WebRtc_UWord32        _targetFrameRate;
    float               _incomingFrameRate;
    WebRtc_UWord32        _maxFrameRate;
    WebRtc_Word64         _incomingFrameTimes[kFrameCountHistorySize];
    bool                _enableTemporalDecimation;

};

} 

#endif
