












#ifndef VPM_VIDEO_DECIMATOR_H
#define VPM_VIDEO_DECIMATOR_H

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMVideoDecimator
{
public:
    VPMVideoDecimator();
    ~VPMVideoDecimator();
    
    void Reset();
    
    void EnableTemporalDecimation(bool enable);
    
    int32_t SetMaxFrameRate(uint32_t maxFrameRate);
    int32_t SetTargetFrameRate(uint32_t frameRate);

    bool DropFrame();
    
    void UpdateIncomingFrameRate();

    
    uint32_t DecimatedFrameRate();

    
    uint32_t InputFrameRate();

private:
    void ProcessIncomingFrameRate(int64_t now);

    enum { kFrameCountHistorySize = 90};
    enum { kFrameHistoryWindowMs = 2000};

    
    int32_t         _overShootModifier;
    uint32_t        _dropCount;
    uint32_t        _keepCount;
    uint32_t        _targetFrameRate;
    float               _incomingFrameRate;
    uint32_t        _maxFrameRate;
    int64_t         _incomingFrameTimes[kFrameCountHistorySize];
    bool                _enableTemporalDecimation;

};

}  

#endif
