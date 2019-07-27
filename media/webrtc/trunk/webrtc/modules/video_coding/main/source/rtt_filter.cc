









#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/rtt_filter.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace webrtc {

VCMRttFilter::VCMRttFilter(int32_t vcmId, int32_t receiverId)
:
_vcmId(vcmId),
_receiverId(receiverId),
_filtFactMax(35),
_jumpStdDevs(2.5),
_driftStdDevs(3.5),
_detectThreshold(kMaxDriftJumpCount)
{
    Reset();
}

VCMRttFilter&
VCMRttFilter::operator=(const VCMRttFilter& rhs)
{
    if (this != &rhs)
    {
        _gotNonZeroUpdate = rhs._gotNonZeroUpdate;
        _avgRtt = rhs._avgRtt;
        _varRtt = rhs._varRtt;
        _maxRtt = rhs._maxRtt;
        _filtFactCount = rhs._filtFactCount;
        _jumpCount = rhs._jumpCount;
        _driftCount = rhs._driftCount;
        memcpy(_jumpBuf, rhs._jumpBuf, sizeof(_jumpBuf));
        memcpy(_driftBuf, rhs._driftBuf, sizeof(_driftBuf));
    }
    return *this;
}

void
VCMRttFilter::Reset()
{
    _gotNonZeroUpdate = false;
    _avgRtt = 0;
    _varRtt = 0;
    _maxRtt = 0;
    _filtFactCount = 1;
    _jumpCount = 0;
    _driftCount = 0;
    memset(_jumpBuf, 0, kMaxDriftJumpCount);
    memset(_driftBuf, 0, kMaxDriftJumpCount);
}

void
VCMRttFilter::Update(uint32_t rttMs)
{
    if (!_gotNonZeroUpdate)
    {
        if (rttMs == 0)
        {
            return;
        }
        _gotNonZeroUpdate = true;
    }

    
    if (rttMs > 3000)
    {
        rttMs = 3000;
    }

    double filtFactor = 0;
    if (_filtFactCount > 1)
    {
        filtFactor = static_cast<double>(_filtFactCount - 1) / _filtFactCount;
    }
    _filtFactCount++;
    if (_filtFactCount > _filtFactMax)
    {
        
        
        
        _filtFactCount = _filtFactMax;
    }
    double oldAvg = _avgRtt;
    double oldVar = _varRtt;
    _avgRtt = filtFactor * _avgRtt + (1 - filtFactor) * rttMs;
    _varRtt = filtFactor * _varRtt + (1 - filtFactor) *
                (rttMs - _avgRtt) * (rttMs - _avgRtt);
    _maxRtt = VCM_MAX(rttMs, _maxRtt);
    if (!JumpDetection(rttMs) || !DriftDetection(rttMs))
    {
        
        _avgRtt = oldAvg;
        _varRtt = oldVar;
    }
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "RttFilter Update: sample=%u avgRtt=%f varRtt=%f maxRtt=%u",
               rttMs, _avgRtt, _varRtt, _maxRtt);
}

bool
VCMRttFilter::JumpDetection(uint32_t rttMs)
{
    double diffFromAvg = _avgRtt - rttMs;
    if (fabs(diffFromAvg) > _jumpStdDevs * sqrt(_varRtt))
    {
        int diffSign = (diffFromAvg >= 0) ? 1 : -1;
        int jumpCountSign = (_jumpCount >= 0) ? 1 : -1;
        if (diffSign != jumpCountSign)
        {
            
            
            
            _jumpCount = 0;
        }
        if (abs(_jumpCount) < kMaxDriftJumpCount)
        {
            
            
            
            
            
            _jumpBuf[abs(_jumpCount)] = rttMs;
            _jumpCount += diffSign;
        }
        if (abs(_jumpCount) >= _detectThreshold)
        {
            
            ShortRttFilter(_jumpBuf, abs(_jumpCount));
            _filtFactCount = _detectThreshold + 1;
            _jumpCount = 0;
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
                       "Detected an RTT jump");
        }
        else
        {
            return false;
        }
    }
    else
    {
        _jumpCount = 0;
    }
    return true;
}

bool
VCMRttFilter::DriftDetection(uint32_t rttMs)
{
    if (_maxRtt - _avgRtt > _driftStdDevs * sqrt(_varRtt))
    {
        if (_driftCount < kMaxDriftJumpCount)
        {
            
            
            _driftBuf[_driftCount] = rttMs;
            _driftCount++;
        }
        if (_driftCount >= _detectThreshold)
        {
            
            ShortRttFilter(_driftBuf, _driftCount);
            _filtFactCount = _detectThreshold + 1;
            _driftCount = 0;
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
                       "Detected an RTT drift");
        }
    }
    else
    {
        _driftCount = 0;
    }
    return true;
}

void
VCMRttFilter::ShortRttFilter(uint32_t* buf, uint32_t length)
{
    if (length == 0)
    {
        return;
    }
    _maxRtt = 0;
    _avgRtt = 0;
    for (uint32_t i=0; i < length; i++)
    {
        if (buf[i] > _maxRtt)
        {
            _maxRtt = buf[i];
        }
        _avgRtt += buf[i];
    }
    _avgRtt = _avgRtt / static_cast<double>(length);
}

uint32_t
VCMRttFilter::RttMs() const
{
    return static_cast<uint32_t>(_maxRtt + 0.5);
}

}
