









#include "modules/remote_bitrate_estimator/remote_rate_control.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#if _WIN32
#include <windows.h>
#endif

#include "system_wrappers/interface/trace.h"

#ifdef MATLAB
extern MatlabEngine eng; 
#endif

namespace webrtc {

const unsigned int kDefaultRttMs = 200;

RemoteRateControl::RemoteRateControl()
:
_minConfiguredBitRate(30000),
_maxConfiguredBitRate(30000000),
_currentBitRate(_maxConfiguredBitRate),
_maxHoldRate(0),
_avgMaxBitRate(-1.0f),
_varMaxBitRate(0.4f),
_rcState(kRcHold),
_cameFromState(kRcDecrease),
_rcRegion(kRcMaxUnknown),
_lastBitRateChange(-1),
_currentInput(kBwNormal, 0, 1.0),
_updated(false),
_timeFirstIncomingEstimate(-1),
_initializedBitRate(false),
_avgChangePeriod(1000.0f),
_lastChangeMs(-1),
_beta(0.9f),
_rtt(kDefaultRttMs)
#ifdef MATLAB
,_plot1(NULL),
_plot2(NULL)
#endif
{
}

RemoteRateControl::~RemoteRateControl()
{
#ifdef MATLAB
    eng.DeletePlot(_plot1);
    eng.DeletePlot(_plot2);
#endif
}

void RemoteRateControl::Reset()
{
    _minConfiguredBitRate = 30000;
    _maxConfiguredBitRate = 30000000;
    _currentBitRate = _maxConfiguredBitRate;
    _maxHoldRate = 0;
    _avgMaxBitRate = -1.0f;
    _varMaxBitRate = 0.4f;
    _rcState = kRcHold;
    _cameFromState = kRcHold;
    _rcRegion = kRcMaxUnknown;
    _lastBitRateChange = -1;
    _avgChangePeriod = 1000.0f;
    _lastChangeMs = -1;
    _beta = 0.9f;
    _currentInput._bwState = kBwNormal;
    _currentInput._incomingBitRate = 0;
    _currentInput._noiseVar = 1.0;
    _updated = false;
    _timeFirstIncomingEstimate = -1;
    _initializedBitRate = false;
}

bool RemoteRateControl::ValidEstimate() const {
  return _initializedBitRate;
}

bool RemoteRateControl::TimeToReduceFurther(
    int64_t time_now, unsigned int incoming_bitrate) const {
  const int bitrate_reduction_interval = BWE_MAX(BWE_MIN(_rtt, 200), 10);
  if (time_now - _lastBitRateChange >= bitrate_reduction_interval) {
    return true;
  }
  if (ValidEstimate()) {
    const int threshold = static_cast<int>(1.05 * incoming_bitrate);
    const int bitrate_difference = LatestEstimate() - incoming_bitrate;
    return bitrate_difference > threshold;
  }
  return false;
}

int32_t RemoteRateControl::SetConfiguredBitRates(
    uint32_t minBitRateBps, uint32_t maxBitRateBps)
{
    if (minBitRateBps > maxBitRateBps)
    {
        return -1;
    }
    _minConfiguredBitRate = minBitRateBps;
    _maxConfiguredBitRate = maxBitRateBps;
    _currentBitRate = BWE_MIN(BWE_MAX(minBitRateBps, _currentBitRate),
                              maxBitRateBps);
    return 0;
}

uint32_t RemoteRateControl::LatestEstimate() const {
  return _currentBitRate;
}

uint32_t RemoteRateControl::UpdateBandwidthEstimate(int64_t nowMS)
{
    _currentBitRate = ChangeBitRate(_currentBitRate,
                                    _currentInput._incomingBitRate,
                                    _currentInput._noiseVar,
                                    nowMS);
    return _currentBitRate;
}

void RemoteRateControl::SetRtt(unsigned int rtt) {
  _rtt = rtt;
}

RateControlRegion RemoteRateControl::Update(const RateControlInput* input,
                                            int64_t nowMS)
{
    assert(input);
#ifdef MATLAB
    
    if (_plot1 == NULL)
    {
        _plot1 = eng.NewPlot(new MatlabPlot());

        _plot1->AddTimeLine(30, "b", "current");
        _plot1->AddTimeLine(30, "r-", "avgMax");
        _plot1->AddTimeLine(30, "r--", "pStdMax");
        _plot1->AddTimeLine(30, "r--", "nStdMax");
        _plot1->AddTimeLine(30, "r+", "max");
        _plot1->AddTimeLine(30, "g", "incoming");
        _plot1->AddTimeLine(30, "b+", "recovery");
    }
    if (_plot2 == NULL)
    {
        _plot2 = eng.NewPlot(new MatlabPlot());

        _plot2->AddTimeLine(30, "b", "alpha");
    }
#endif

    
    
    if (!_initializedBitRate)
    {
        if (_timeFirstIncomingEstimate < 0)
        {
            if (input->_incomingBitRate > 0)
            {
                _timeFirstIncomingEstimate = nowMS;
            }
        }
        else if (nowMS - _timeFirstIncomingEstimate > 500 &&
            input->_incomingBitRate > 0)
        {
            _currentBitRate = input->_incomingBitRate;
            _initializedBitRate = true;
        }
    }

    if (_updated && _currentInput._bwState == kBwOverusing)
    {
        
        _currentInput._noiseVar = input->_noiseVar;
        _currentInput._incomingBitRate = input->_incomingBitRate;
        return _rcRegion;
    }
    _updated = true;
    _currentInput = *input;
    WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1, "BWE: Incoming rate = %u kbps", input->_incomingBitRate/1000);
    return _rcRegion;
}

uint32_t RemoteRateControl::ChangeBitRate(uint32_t currentBitRate,
                                                uint32_t incomingBitRate,
                                                double noiseVar,
                                                int64_t nowMS)
{
    if (!_updated)
    {
        return _currentBitRate;
    }
    _updated = false;
    UpdateChangePeriod(nowMS);
    ChangeState(_currentInput, nowMS);
    
    const float incomingBitRateKbps = incomingBitRate / 1000.0f;
    
    
    const float stdMaxBitRate = sqrt(_varMaxBitRate * _avgMaxBitRate);
    bool recovery = false;
    switch (_rcState)
    {
    case kRcHold:
        {
            _maxHoldRate = BWE_MAX(_maxHoldRate, incomingBitRate);
            break;
        }
    case kRcIncrease:
        {
            if (_avgMaxBitRate >= 0)
            {
                if (incomingBitRateKbps > _avgMaxBitRate + 3 * stdMaxBitRate)
                {
                    ChangeRegion(kRcMaxUnknown);
                    _avgMaxBitRate = -1.0;
                }
                else if (incomingBitRateKbps > _avgMaxBitRate + 2.5 * stdMaxBitRate)
                {
                    ChangeRegion(kRcAboveMax);
                }
            }
            WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                         "BWE: Response time: %f + %i + 10*33\n",
                         _avgChangePeriod, _rtt);
            const uint32_t responseTime = static_cast<uint32_t>(_avgChangePeriod + 0.5f) + _rtt + 300;
            double alpha = RateIncreaseFactor(nowMS, _lastBitRateChange,
                                              responseTime, noiseVar);

            WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                "BWE: _avgChangePeriod = %f ms; RTT = %u ms", _avgChangePeriod, _rtt);

            currentBitRate = static_cast<uint32_t>(currentBitRate * alpha) + 1000;
            if (_maxHoldRate > 0 && _beta * _maxHoldRate > currentBitRate)
            {
                currentBitRate = static_cast<uint32_t>(_beta * _maxHoldRate);
                _avgMaxBitRate = _beta * _maxHoldRate / 1000.0f;
                ChangeRegion(kRcNearMax);
                recovery = true;
#ifdef MATLAB
                _plot1->Append("recovery", _maxHoldRate/1000);
#endif
            }
            _maxHoldRate = 0;
            WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                "BWE: Increase rate to currentBitRate = %u kbps", currentBitRate/1000);
            _lastBitRateChange = nowMS;
            break;
        }
    case kRcDecrease:
        {
            if (incomingBitRate < _minConfiguredBitRate)
            {
                currentBitRate = _minConfiguredBitRate;
            }
            else
            {
                
                
                currentBitRate = static_cast<uint32_t>(_beta * incomingBitRate + 0.5);
                if (currentBitRate > _currentBitRate)
                {
                    
                    if (_rcRegion != kRcMaxUnknown)
                    {
                        currentBitRate = static_cast<uint32_t>(_beta * _avgMaxBitRate * 1000 + 0.5f);
                    }
                    currentBitRate = BWE_MIN(currentBitRate, _currentBitRate);
                }
                ChangeRegion(kRcNearMax);

                if (incomingBitRateKbps < _avgMaxBitRate - 3 * stdMaxBitRate)
                {
                    _avgMaxBitRate = -1.0f;
                }

                UpdateMaxBitRateEstimate(incomingBitRateKbps);

#ifdef MATLAB
                _plot1->Append("max", incomingBitRateKbps);
#endif

                WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1, "BWE: Decrease rate to currentBitRate = %u kbps", currentBitRate/1000);
            }
            
            ChangeState(kRcHold);
            _lastBitRateChange = nowMS;
            break;
        }
    }
    if (!recovery && (incomingBitRate > 100000 || currentBitRate > 150000) &&
        currentBitRate > 1.5 * incomingBitRate)
    {
        
        
        currentBitRate = _currentBitRate;
        _lastBitRateChange = nowMS;
    }
#ifdef MATLAB
    if (_avgMaxBitRate >= 0.0f)
    {
        _plot1->Append("avgMax", _avgMaxBitRate);
        _plot1->Append("pStdMax", _avgMaxBitRate + 3*stdMaxBitRate);
        _plot1->Append("nStdMax", _avgMaxBitRate - 3*stdMaxBitRate);
    }
    _plot1->Append("incoming", incomingBitRate/1000);
    _plot1->Append("current", currentBitRate/1000);
    _plot1->Plot();
#endif
    return currentBitRate;
}

double RemoteRateControl::RateIncreaseFactor(int64_t nowMs, int64_t lastMs, uint32_t reactionTimeMs, double noiseVar) const
{
    
    
    const double B = 0.0407;
    const double b = 0.0025;
    const double c1 = -6700.0 / (33 * 33);
    const double c2 = 800.0;
    const double d = 0.85;

    double alpha = 1.005 + B / (1 + exp( b * (d * reactionTimeMs - (c1 * noiseVar + c2))));

    if (alpha < 1.005)
    {
        alpha = 1.005;
    }
    else if (alpha > 1.3)
    {
        alpha = 1.3;
    }

    WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
        "BWE: alpha = %f", alpha);
#ifdef MATLAB
            _plot2->Append("alpha", alpha);
            _plot2->Plot();
#endif

    if (lastMs > -1)
    {
        alpha = pow(alpha, (nowMs - lastMs) / 1000.0);
    }

    if (_rcRegion == kRcNearMax)
    {
        
        
        alpha = alpha - (alpha - 1.0) / 2.0;
    }
    else if (_rcRegion == kRcMaxUnknown)
    {
        alpha = alpha + (alpha - 1.0) * 2.0;
    }

    return alpha;
}

void RemoteRateControl::UpdateChangePeriod(int64_t nowMs)
{
    int64_t changePeriod = 0;
    if (_lastChangeMs > -1)
    {
        changePeriod = nowMs - _lastChangeMs;
    }
    _lastChangeMs = nowMs;
    _avgChangePeriod = 0.9f * _avgChangePeriod + 0.1f * changePeriod;
}

void RemoteRateControl::UpdateMaxBitRateEstimate(float incomingBitRateKbps)
{
    const float alpha = 0.05f;
    if (_avgMaxBitRate == -1.0f)
    {
        _avgMaxBitRate = incomingBitRateKbps;
    }
    else
    {
        _avgMaxBitRate = (1 - alpha) * _avgMaxBitRate +
                            alpha * incomingBitRateKbps;
    }
    
    
    const float norm = BWE_MAX(_avgMaxBitRate, 1.0f);
    _varMaxBitRate = (1 - alpha) * _varMaxBitRate +
               alpha * (_avgMaxBitRate - incomingBitRateKbps) *
                       (_avgMaxBitRate - incomingBitRateKbps) /
                       norm;
    
    if (_varMaxBitRate < 0.4f)
    {
        _varMaxBitRate = 0.4f;
    }
    
    if (_varMaxBitRate > 2.5f)
    {
        _varMaxBitRate = 2.5f;
    }
}

void RemoteRateControl::ChangeState(const RateControlInput& input, int64_t nowMs)
{
    switch (_currentInput._bwState)
    {
    case kBwNormal:
        {
            if (_rcState == kRcHold)
            {
                _lastBitRateChange = nowMs;
                ChangeState(kRcIncrease);
            }
            break;
        }
    case kBwOverusing:
        {
            if (_rcState != kRcDecrease)
            {
                ChangeState(kRcDecrease);
            }
            break;
        }
    case kBwUnderusing:
        {
            ChangeState(kRcHold);
            break;
        }
    }
}

void RemoteRateControl::ChangeRegion(RateControlRegion region)
{
    _rcRegion = region;
    switch (_rcRegion)
    {
    case kRcAboveMax:
    case kRcMaxUnknown:
        {
            _beta = 0.9f;
            break;
        }
    case kRcNearMax:
        {
            _beta = 0.95f;
            break;
        }
    }
}

void RemoteRateControl::ChangeState(RateControlState newState)
{
    _cameFromState = _rcState;
    _rcState = newState;
    char state1[15];
    char state2[15];
    char state3[15];
    StateStr(_cameFromState, state1);
    StateStr(_rcState, state2);
    StateStr(_currentInput._bwState, state3);
    WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                 "\t%s => %s due to %s\n", state1, state2, state3);
}

void RemoteRateControl::StateStr(RateControlState state, char* str)
{
    switch (state)
    {
    case kRcDecrease:
        strncpy(str, "DECREASE", 9);
        break;
    case kRcHold:
        strncpy(str, "HOLD", 5);
        break;
    case kRcIncrease:
        strncpy(str, "INCREASE", 9);
        break;
    }
}

void RemoteRateControl::StateStr(BandwidthUsage state, char* str)
{
    switch (state)
    {
    case kBwNormal:
        strncpy(str, "NORMAL", 7);
        break;
    case kBwOverusing:
        strncpy(str, "OVER USING", 11);
        break;
    case kBwUnderusing:
        strncpy(str, "UNDER USING", 12);
        break;
    }
}

} 
