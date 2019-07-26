









#ifndef WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_
#define WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_

#include "webrtc/typedefs.h"

namespace webrtc
{

class VCMRttFilter
{
public:
    VCMRttFilter(int32_t vcmId = 0, int32_t receiverId = 0);

    VCMRttFilter& operator=(const VCMRttFilter& rhs);

    
    void Reset();
    
    void Update(uint32_t rttMs);
    
    uint32_t RttMs() const;

private:
    
    
    
    enum { kMaxDriftJumpCount = 5 };
    
    
    
    
    bool JumpDetection(uint32_t rttMs);
    
    
    
    
    bool DriftDetection(uint32_t rttMs);
    
    void ShortRttFilter(uint32_t* buf, uint32_t length);

    int32_t         _vcmId;
    int32_t         _receiverId;
    bool                  _gotNonZeroUpdate;
    double                _avgRtt;
    double                _varRtt;
    uint32_t        _maxRtt;
    uint32_t        _filtFactCount;
    const uint32_t  _filtFactMax;
    const double          _jumpStdDevs;
    const double          _driftStdDevs;
    int32_t         _jumpCount;
    int32_t         _driftCount;
    const int32_t   _detectThreshold;
    uint32_t        _jumpBuf[kMaxDriftJumpCount];
    uint32_t        _driftBuf[kMaxDriftJumpCount];
};

}  

#endif
