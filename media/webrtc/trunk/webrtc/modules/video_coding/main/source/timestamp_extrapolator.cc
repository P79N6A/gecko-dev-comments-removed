









#include "internal_defines.h"
#include "timestamp_extrapolator.h"
#include "trace.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {

VCMTimestampExtrapolator::VCMTimestampExtrapolator(Clock* clock,
                                                   int32_t vcmId,
                                                   int32_t id)
:
_rwLock(RWLockWrapper::CreateRWLock()),
_vcmId(vcmId),
_id(id),
_clock(clock),
_startMs(0),
_firstTimestamp(0),
_wrapArounds(0),
_prevTs90khz(0),
_lambda(1),
_firstAfterReset(true),
_packetCount(0),
_startUpFilterDelayInPackets(2),
_detectorAccumulatorPos(0),
_detectorAccumulatorNeg(0),
_alarmThreshold(60e3),
_accDrift(6600), 
_accMaxError(7000),
_P11(1e10)
{
    Reset(_clock->TimeInMilliseconds());
}

VCMTimestampExtrapolator::~VCMTimestampExtrapolator()
{
    delete _rwLock;
}

void
VCMTimestampExtrapolator::Reset(const int64_t nowMs )
{
    WriteLockScoped wl(*_rwLock);
    if (nowMs > -1)
    {
        _startMs = nowMs;
    }
    else
    {
        _startMs = _clock->TimeInMilliseconds();
    }
    _prevMs = _startMs;
    _firstTimestamp = 0;
    _w[0] = 90.0;
    _w[1] = 0;
    _pp[0][0] = 1;
    _pp[1][1] = _P11;
    _pp[0][1] = _pp[1][0] = 0;
    _firstAfterReset = true;
    _prevTs90khz = 0;
    _wrapArounds = 0;
    _packetCount = 0;
    _detectorAccumulatorPos = 0;
    _detectorAccumulatorNeg = 0;
}

void
VCMTimestampExtrapolator::Update(int64_t tMs, uint32_t ts90khz, bool trace)
{

    _rwLock->AcquireLockExclusive();
    if (tMs - _prevMs > 10e3)
    {
        
        
        _rwLock->ReleaseLockExclusive();
        Reset();
        _rwLock->AcquireLockExclusive();
    }
    else
    {
        _prevMs = tMs;
    }

    
    tMs -= _startMs;

    int32_t prevWrapArounds = _wrapArounds;
    CheckForWrapArounds(ts90khz);
    int32_t wrapAroundsSincePrev = _wrapArounds - prevWrapArounds;

    if (wrapAroundsSincePrev == 0 && ts90khz < _prevTs90khz)
    {
        _rwLock->ReleaseLockExclusive();
        return;
    }

    if (_firstAfterReset)
    {
        
        
        
        _w[1] = -_w[0] * tMs;
        _firstTimestamp = ts90khz;
        _firstAfterReset = false;
    }

    
    _w[1] = _w[1] - wrapAroundsSincePrev * ((static_cast<int64_t>(1)<<32) - 1);

    double residual = (static_cast<double>(ts90khz) - _firstTimestamp) - static_cast<double>(tMs) * _w[0] - _w[1];
    if (DelayChangeDetection(residual, trace) &&
        _packetCount >= _startUpFilterDelayInPackets)
    {
        
        
        
        _pp[1][1] = _P11;
    }
    
    
    
    double K[2];
    K[0] = _pp[0][0] * tMs + _pp[0][1];
    K[1] = _pp[1][0] * tMs + _pp[1][1];
    double TPT = _lambda + tMs * K[0] + K[1];
    K[0] /= TPT;
    K[1] /= TPT;
    
    _w[0] = _w[0] + K[0] * residual;
    _w[1] = _w[1] + K[1] * residual;
    
    double p00 = 1 / _lambda * (_pp[0][0] - (K[0] * tMs * _pp[0][0] + K[0] * _pp[1][0]));
    double p01 = 1 / _lambda * (_pp[0][1] - (K[0] * tMs * _pp[0][1] + K[0] * _pp[1][1]));
    _pp[1][0] = 1 / _lambda * (_pp[1][0] - (K[1] * tMs * _pp[0][0] + K[1] * _pp[1][0]));
    _pp[1][1] = 1 / _lambda * (_pp[1][1] - (K[1] * tMs * _pp[0][1] + K[1] * _pp[1][1]));
    _pp[0][0] = p00;
    _pp[0][1] = p01;
    if (_packetCount < _startUpFilterDelayInPackets)
    {
        _packetCount++;
    }
    if (trace)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _id),  "w[0]=%f w[1]=%f ts=%u tMs=%u", _w[0], _w[1], ts90khz, tMs);
    }
    _rwLock->ReleaseLockExclusive();
}

uint32_t
VCMTimestampExtrapolator::ExtrapolateTimestamp(int64_t tMs) const
{
    ReadLockScoped rl(*_rwLock);
    uint32_t timestamp = 0;
    if (_packetCount == 0)
    {
        timestamp = 0;
    }
    else if (_packetCount < _startUpFilterDelayInPackets)
    {
        timestamp = static_cast<uint32_t>(90.0 * (tMs - _prevMs) + _prevTs90khz + 0.5);
    }
    else
    {
        timestamp = static_cast<uint32_t>(_w[0] * (tMs - _startMs) + _w[1] + _firstTimestamp + 0.5);
    }
    return timestamp;
}

int64_t
VCMTimestampExtrapolator::ExtrapolateLocalTime(uint32_t timestamp90khz) const
{
    ReadLockScoped rl(*_rwLock);
    int64_t localTimeMs = 0;
    if (_packetCount == 0)
    {
        localTimeMs = -1;
    }
    else if (_packetCount < _startUpFilterDelayInPackets)
    {
        localTimeMs = _prevMs + static_cast<int64_t>(static_cast<double>(timestamp90khz - _prevTs90khz) / 90.0 + 0.5);
    }
    else
    {
        if (_w[0] < 1e-3)
        {
            localTimeMs = _startMs;
        }
        else
        {
            double timestampDiff = static_cast<double>(timestamp90khz) - static_cast<double>(_firstTimestamp);
            localTimeMs = static_cast<int64_t>(static_cast<double>(_startMs) + (timestampDiff - _w[1]) / _w[0] + 0.5);
        }
    }
    return localTimeMs;
}



void
VCMTimestampExtrapolator::CheckForWrapArounds(uint32_t ts90khz)
{
    if (_prevTs90khz == 0)
    {
        _prevTs90khz = ts90khz;
        return;
    }
    if (ts90khz < _prevTs90khz)
    {
        
        
        
        if (static_cast<int32_t>(ts90khz - _prevTs90khz) > 0)
        {
            
            _wrapArounds++;
        }
    }
    
    
    else if (static_cast<int32_t>(_prevTs90khz - ts90khz) > 0)
    {
        
        _wrapArounds--;
    }
    _prevTs90khz = ts90khz;
}

bool
VCMTimestampExtrapolator::DelayChangeDetection(double error, bool trace)
{
    
    error = (error > 0) ? VCM_MIN(error, _accMaxError) : VCM_MAX(error, -_accMaxError);
    _detectorAccumulatorPos = VCM_MAX(_detectorAccumulatorPos + error - _accDrift, (double)0);
    _detectorAccumulatorNeg = VCM_MIN(_detectorAccumulatorNeg + error + _accDrift, (double)0);
    if (_detectorAccumulatorPos > _alarmThreshold || _detectorAccumulatorNeg < -_alarmThreshold)
    {
        
        if (trace)
        {
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _id),  "g1=%f g2=%f alarm=1", _detectorAccumulatorPos, _detectorAccumulatorNeg);
        }
        _detectorAccumulatorPos = _detectorAccumulatorNeg = 0;
        return true;
    }
    if (trace)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _id),  "g1=%f g2=%f alarm=0", _detectorAccumulatorPos, _detectorAccumulatorNeg);
    }
    return false;
}

}
