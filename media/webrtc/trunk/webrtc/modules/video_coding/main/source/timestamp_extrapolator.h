









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_EXTRAPOLATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_EXTRAPOLATOR_H_

#include "typedefs.h"
#include "rw_lock_wrapper.h"

namespace webrtc
{

class Clock;

class VCMTimestampExtrapolator
{
public:
    VCMTimestampExtrapolator(Clock* clock,
                             int32_t vcmId = 0,
                             int32_t receiverId = 0);
    ~VCMTimestampExtrapolator();
    void Update(int64_t tMs, uint32_t ts90khz, bool trace = true);
    uint32_t ExtrapolateTimestamp(int64_t tMs) const;
    int64_t ExtrapolateLocalTime(uint32_t timestamp90khz) const;
    void Reset(int64_t nowMs = -1);

private:
    void CheckForWrapArounds(uint32_t ts90khz);
    bool DelayChangeDetection(double error, bool trace = true);
    RWLockWrapper*        _rwLock;
    int32_t         _vcmId;
    int32_t         _id;
    Clock*                _clock;
    double                _w[2];
    double                _pp[2][2];
    int64_t         _startMs;
    int64_t         _prevMs;
    uint32_t        _firstTimestamp;
    int32_t         _wrapArounds;
    uint32_t        _prevTs90khz;
    const double          _lambda;
    bool                  _firstAfterReset;
    uint32_t        _packetCount;
    const uint32_t  _startUpFilterDelayInPackets;

    double              _detectorAccumulatorPos;
    double              _detectorAccumulatorNeg;
    const double        _alarmThreshold;
    const double        _accDrift;
    const double        _accMaxError;
    const double        _P11;
};

} 

#endif 
