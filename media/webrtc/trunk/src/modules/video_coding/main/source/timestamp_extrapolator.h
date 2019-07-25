









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_EXTRAPOLATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_EXTRAPOLATOR_H_

#include "typedefs.h"
#include "rw_lock_wrapper.h"

namespace webrtc
{

class TickTimeBase;

class VCMTimestampExtrapolator
{
public:
    VCMTimestampExtrapolator(TickTimeBase* clock,
                             WebRtc_Word32 vcmId = 0,
                             WebRtc_Word32 receiverId = 0);
    ~VCMTimestampExtrapolator();
    void Update(WebRtc_Word64 tMs, WebRtc_UWord32 ts90khz, bool trace = true);
    WebRtc_UWord32 ExtrapolateTimestamp(WebRtc_Word64 tMs) const;
    WebRtc_Word64 ExtrapolateLocalTime(WebRtc_UWord32 timestamp90khz) const;
    void Reset(WebRtc_Word64 nowMs = -1);

private:
    void CheckForWrapArounds(WebRtc_UWord32 ts90khz);
    bool DelayChangeDetection(double error, bool trace = true);
    RWLockWrapper*        _rwLock;
    WebRtc_Word32         _vcmId;
    WebRtc_Word32         _id;
    TickTimeBase*         _clock;
    double              _w[2];
    double              _P[2][2];
    WebRtc_Word64         _startMs;
    WebRtc_Word64         _prevMs;
    WebRtc_UWord32        _firstTimestamp;
    WebRtc_Word32         _wrapArounds;
    WebRtc_UWord32        _prevTs90khz;
    const double        _lambda;
    bool                _firstAfterReset;
    WebRtc_UWord32        _packetCount;
    const WebRtc_UWord32  _startUpFilterDelayInPackets;

    double              _detectorAccumulatorPos;
    double              _detectorAccumulatorNeg;
    const double        _alarmThreshold;
    const double        _accDrift;
    const double        _accMaxError;
    const double        _P11;
};

} 

#endif 
