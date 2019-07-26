









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_REMOTE_RATE_CONTROL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_REMOTE_RATE_CONTROL_H_

#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "typedefs.h"

#ifdef MATLAB
#include "../test/BWEStandAlone/MatlabPlot.h"
#endif

namespace webrtc {
class RemoteRateControl
{
public:
    RemoteRateControl();
    ~RemoteRateControl();
    int32_t SetConfiguredBitRates(uint32_t minBitRate,
                                        uint32_t maxBitRate);
    uint32_t LatestEstimate() const;
    uint32_t UpdateBandwidthEstimate(int64_t nowMS);
    void SetRtt(unsigned int rtt);
    RateControlRegion Update(const RateControlInput* input,
                             int64_t nowMS);
    void Reset();

    
    
    bool ValidEstimate() const;
    
    
    
    
    bool TimeToReduceFurther(int64_t time_now,
                             unsigned int incoming_bitrate) const;

private:
    uint32_t ChangeBitRate(uint32_t currentBitRate,
                                 uint32_t incomingBitRate,
                                 double delayFactor,
                                 int64_t nowMS);
    double RateIncreaseFactor(int64_t nowMs,
                              int64_t lastMs,
                              uint32_t reactionTimeMs,
                              double noiseVar) const;
    void UpdateChangePeriod(int64_t nowMs);
    void UpdateMaxBitRateEstimate(float incomingBitRateKbps);
    void ChangeState(const RateControlInput& input, int64_t nowMs);
    void ChangeState(RateControlState newState);
    void ChangeRegion(RateControlRegion region);
    static void StateStr(RateControlState state, char* str);
    static void StateStr(BandwidthUsage state, char* str);

    uint32_t        _minConfiguredBitRate;
    uint32_t        _maxConfiguredBitRate;
    uint32_t        _currentBitRate;
    uint32_t        _maxHoldRate;
    float               _avgMaxBitRate;
    float               _varMaxBitRate;
    RateControlState    _rcState;
    RateControlState    _cameFromState;
    RateControlRegion   _rcRegion;
    int64_t         _lastBitRateChange;
    RateControlInput    _currentInput;
    bool                _updated;
    int64_t         _timeFirstIncomingEstimate;
    bool                _initializedBitRate;

    float               _avgChangePeriod;
    int64_t         _lastChangeMs;
    float               _beta;
    unsigned int _rtt;
#ifdef MATLAB
    MatlabPlot          *_plot1;
    MatlabPlot          *_plot2;
#endif
};
} 

#endif 
