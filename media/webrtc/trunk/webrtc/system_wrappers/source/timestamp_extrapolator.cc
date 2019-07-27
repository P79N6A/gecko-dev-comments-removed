









#include "webrtc/system_wrappers/interface/timestamp_extrapolator.h"

#include <algorithm>

namespace webrtc {

TimestampExtrapolator::TimestampExtrapolator(int64_t start_ms)
    : _rwLock(RWLockWrapper::CreateRWLock()),
      _startMs(0),
      _firstTimestamp(0),
      _wrapArounds(0),
      _prevUnwrappedTimestamp(-1),
      _prevWrapTimestamp(-1),
      _lambda(1),
      _firstAfterReset(true),
      _packetCount(0),
      _startUpFilterDelayInPackets(2),
      _detectorAccumulatorPos(0),
      _detectorAccumulatorNeg(0),
      _alarmThreshold(60e3),
      _accDrift(6600),  
      _accMaxError(7000),
      _pP11(1e10) {
    Reset(start_ms);
}

TimestampExtrapolator::~TimestampExtrapolator()
{
    delete _rwLock;
}

void TimestampExtrapolator::Reset(int64_t start_ms)
{
    WriteLockScoped wl(*_rwLock);
    _startMs = start_ms;
    _prevMs = _startMs;
    _firstTimestamp = 0;
    _w[0] = 90.0;
    _w[1] = 0;
    _pP[0][0] = 1;
    _pP[1][1] = _pP11;
    _pP[0][1] = _pP[1][0] = 0;
    _firstAfterReset = true;
    _prevUnwrappedTimestamp = -1;
    _prevWrapTimestamp = -1;
    _wrapArounds = 0;
    _packetCount = 0;
    _detectorAccumulatorPos = 0;
    _detectorAccumulatorNeg = 0;
}

void
TimestampExtrapolator::Update(int64_t tMs, uint32_t ts90khz)
{

    _rwLock->AcquireLockExclusive();
    if (tMs - _prevMs > 10e3)
    {
        
        
        _rwLock->ReleaseLockExclusive();
        Reset(tMs);
        _rwLock->AcquireLockExclusive();
    }
    else
    {
        _prevMs = tMs;
    }

    
    tMs -= _startMs;

    CheckForWrapArounds(ts90khz);

    int64_t unwrapped_ts90khz = static_cast<int64_t>(ts90khz) +
        _wrapArounds * ((static_cast<int64_t>(1) << 32) - 1);

    if (_prevUnwrappedTimestamp >= 0 &&
        unwrapped_ts90khz < _prevUnwrappedTimestamp)
    {
        
        _rwLock->ReleaseLockExclusive();
        return;
    }

    if (_firstAfterReset)
    {
        
        
        
        _w[1] = -_w[0] * tMs;
        _firstTimestamp = unwrapped_ts90khz;
        _firstAfterReset = false;
    }

    double residual =
        (static_cast<double>(unwrapped_ts90khz) - _firstTimestamp) -
        static_cast<double>(tMs) * _w[0] - _w[1];
    if (DelayChangeDetection(residual) &&
        _packetCount >= _startUpFilterDelayInPackets)
    {
        
        
        
        _pP[1][1] = _pP11;
    }
    
    
    
    double K[2];
    K[0] = _pP[0][0] * tMs + _pP[0][1];
    K[1] = _pP[1][0] * tMs + _pP[1][1];
    double TPT = _lambda + tMs * K[0] + K[1];
    K[0] /= TPT;
    K[1] /= TPT;
    
    _w[0] = _w[0] + K[0] * residual;
    _w[1] = _w[1] + K[1] * residual;
    
    double p00 = 1 / _lambda *
        (_pP[0][0] - (K[0] * tMs * _pP[0][0] + K[0] * _pP[1][0]));
    double p01 = 1 / _lambda *
        (_pP[0][1] - (K[0] * tMs * _pP[0][1] + K[0] * _pP[1][1]));
    _pP[1][0] = 1 / _lambda *
        (_pP[1][0] - (K[1] * tMs * _pP[0][0] + K[1] * _pP[1][0]));
    _pP[1][1] = 1 / _lambda *
        (_pP[1][1] - (K[1] * tMs * _pP[0][1] + K[1] * _pP[1][1]));
    _pP[0][0] = p00;
    _pP[0][1] = p01;
    _prevUnwrappedTimestamp = unwrapped_ts90khz;
    if (_packetCount < _startUpFilterDelayInPackets)
    {
        _packetCount++;
    }
    _rwLock->ReleaseLockExclusive();
}

int64_t
TimestampExtrapolator::ExtrapolateLocalTime(uint32_t timestamp90khz)
{
    ReadLockScoped rl(*_rwLock);
    int64_t localTimeMs = 0;
    CheckForWrapArounds(timestamp90khz);
    double unwrapped_ts90khz = static_cast<double>(timestamp90khz) +
        _wrapArounds * ((static_cast<int64_t>(1) << 32) - 1);
    if (_packetCount == 0)
    {
        localTimeMs = -1;
    }
    else if (_packetCount < _startUpFilterDelayInPackets)
    {
        localTimeMs = _prevMs + static_cast<int64_t>(
            static_cast<double>(unwrapped_ts90khz - _prevUnwrappedTimestamp) /
            90.0 + 0.5);
    }
    else
    {
        if (_w[0] < 1e-3)
        {
            localTimeMs = _startMs;
        }
        else
        {
            double timestampDiff = unwrapped_ts90khz -
                static_cast<double>(_firstTimestamp);
            localTimeMs = static_cast<int64_t>(
                static_cast<double>(_startMs) + (timestampDiff - _w[1]) /
                _w[0] + 0.5);
        }
    }
    return localTimeMs;
}



void
TimestampExtrapolator::CheckForWrapArounds(uint32_t ts90khz)
{
    if (_prevWrapTimestamp == -1)
    {
        _prevWrapTimestamp = ts90khz;
        return;
    }
    if (ts90khz < _prevWrapTimestamp)
    {
        
        
        
        if (static_cast<int32_t>(ts90khz - _prevWrapTimestamp) > 0)
        {
            
            _wrapArounds++;
        }
    }
    
    
    else if (static_cast<int32_t>(_prevWrapTimestamp - ts90khz) > 0)
    {
        
        _wrapArounds--;
    }
    _prevWrapTimestamp = ts90khz;
}

bool
TimestampExtrapolator::DelayChangeDetection(double error)
{
    
    error = (error > 0) ? std::min(error, _accMaxError) :
                          std::max(error, -_accMaxError);
    _detectorAccumulatorPos =
        std::max(_detectorAccumulatorPos + error - _accDrift, (double)0);
    _detectorAccumulatorNeg =
        std::min(_detectorAccumulatorNeg + error + _accDrift, (double)0);
    if (_detectorAccumulatorPos > _alarmThreshold || _detectorAccumulatorNeg < -_alarmThreshold)
    {
        
        _detectorAccumulatorPos = _detectorAccumulatorNeg = 0;
        return true;
    }
    return false;
}

}
