









#ifndef WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_
#define WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_

#include "typedefs.h"

namespace webrtc
{

class VCMRttFilter
{
public:
    VCMRttFilter(WebRtc_Word32 vcmId = 0, WebRtc_Word32 receiverId = 0);

    VCMRttFilter& operator=(const VCMRttFilter& rhs);

    
    void Reset();
    
    void Update(WebRtc_UWord32 rttMs);
    
    WebRtc_UWord32 RttMs() const;

private:
    
    
    
    enum { kMaxDriftJumpCount = 5 };
    
    
    
    
    bool JumpDetection(WebRtc_UWord32 rttMs);
    
    
    
    
    bool DriftDetection(WebRtc_UWord32 rttMs);
    
    void ShortRttFilter(WebRtc_UWord32* buf, WebRtc_UWord32 length);

    WebRtc_Word32         _vcmId;
    WebRtc_Word32         _receiverId;
    bool                  _gotNonZeroUpdate;
    double                _avgRtt;
    double                _varRtt;
    WebRtc_UWord32        _maxRtt;
    WebRtc_UWord32        _filtFactCount;
    const WebRtc_UWord32  _filtFactMax;
    const double          _jumpStdDevs;
    const double          _driftStdDevs;
    WebRtc_Word32         _jumpCount;
    WebRtc_Word32         _driftCount;
    const WebRtc_Word32   _detectThreshold;
    WebRtc_UWord32        _jumpBuf[kMaxDriftJumpCount];
    WebRtc_UWord32        _driftBuf[kMaxDriftJumpCount];
};

} 

#endif
