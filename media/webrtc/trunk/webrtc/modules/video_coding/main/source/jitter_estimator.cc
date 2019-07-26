









#include "trace.h"
#include "internal_defines.h"
#include "jitter_estimator.h"
#include "rtt_filter.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace webrtc {

VCMJitterEstimator::VCMJitterEstimator(WebRtc_Word32 vcmId, WebRtc_Word32 receiverId) :
_vcmId(vcmId),
_receiverId(receiverId),
_phi(0.97),
_psi(0.9999),
_alphaCountMax(400),
_thetaLow(0.000001),
_nackLimit(3),
_numStdDevDelayOutlier(15),
_numStdDevFrameSizeOutlier(3),
_noiseStdDevs(2.33), 
                     
_noiseStdDevOffset(30.0), 
_rttFilter(vcmId, receiverId)
{
    Reset();
}

VCMJitterEstimator&
VCMJitterEstimator::operator=(const VCMJitterEstimator& rhs)
{
    if (this != &rhs)
    {
        memcpy(_thetaCov, rhs._thetaCov, sizeof(_thetaCov));
        memcpy(_Qcov, rhs._Qcov, sizeof(_Qcov));

        _vcmId = rhs._vcmId;
        _receiverId = rhs._receiverId;
        _avgFrameSize = rhs._avgFrameSize;
        _varFrameSize = rhs._varFrameSize;
        _maxFrameSize = rhs._maxFrameSize;
        _fsSum = rhs._fsSum;
        _fsCount = rhs._fsCount;
        _lastUpdateT = rhs._lastUpdateT;
        _prevEstimate = rhs._prevEstimate;
        _prevFrameSize = rhs._prevFrameSize;
        _avgNoise = rhs._avgNoise;
        _alphaCount = rhs._alphaCount;
        _filterJitterEstimate = rhs._filterJitterEstimate;
        _startupCount = rhs._startupCount;
        _latestNackTimestamp = rhs._latestNackTimestamp;
        _nackCount = rhs._nackCount;
        _rttFilter = rhs._rttFilter;
    }
    return *this;
}


void
VCMJitterEstimator::Reset()
{
    _theta[0] = 1/(512e3/8);
    _theta[1] = 0;
    _varNoise = 4.0;

    _thetaCov[0][0] = 1e-4;
    _thetaCov[1][1] = 1e2;
    _thetaCov[0][1] = _thetaCov[1][0] = 0;
    _Qcov[0][0] = 2.5e-10;
    _Qcov[1][1] = 1e-10;
    _Qcov[0][1] = _Qcov[1][0] = 0;
    _avgFrameSize = 500;
    _maxFrameSize = 500;
    _varFrameSize = 100;
    _lastUpdateT = -1;
    _prevEstimate = -1.0;
    _prevFrameSize = 0;
    _avgNoise = 0.0;
    _alphaCount = 1;
    _filterJitterEstimate = 0.0;
    _latestNackTimestamp = 0;
    _nackCount = 0;
    _fsSum = 0;
    _fsCount = 0;
    _startupCount = 0;
    _rttFilter.Reset();
}

void
VCMJitterEstimator::ResetNackCount()
{
    _nackCount = 0;
}


void
VCMJitterEstimator::UpdateEstimate(WebRtc_Word64 frameDelayMS, WebRtc_UWord32 frameSizeBytes,
                                            bool incompleteFrame )
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
               VCMId(_vcmId, _receiverId),
               "Jitter estimate updated with: frameSize=%d frameDelayMS=%d",
               frameSizeBytes, frameDelayMS);
    if (frameSizeBytes == 0)
    {
        return;
    }
    int deltaFS = frameSizeBytes - _prevFrameSize;
    if (_fsCount < kFsAccuStartupSamples)
    {
        _fsSum += frameSizeBytes;
        _fsCount++;
    }
    else if (_fsCount == kFsAccuStartupSamples)
    {
        
        _avgFrameSize = static_cast<double>(_fsSum) /
                        static_cast<double>(_fsCount);
        _fsCount++;
    }
    if (!incompleteFrame || frameSizeBytes > _avgFrameSize)
    {
        double avgFrameSize = _phi * _avgFrameSize +
                              (1 - _phi) * frameSizeBytes;
        if (frameSizeBytes < _avgFrameSize + 2 * sqrt(_varFrameSize))
        {
            
            
            _avgFrameSize = avgFrameSize;
        }
        
        
        _varFrameSize = VCM_MAX(_phi * _varFrameSize + (1 - _phi) *
                                (frameSizeBytes - avgFrameSize) *
                                (frameSizeBytes - avgFrameSize), 1.0);
    }

    
    _maxFrameSize = VCM_MAX(_psi * _maxFrameSize, static_cast<double>(frameSizeBytes));

    if (_prevFrameSize == 0)
    {
        _prevFrameSize = frameSizeBytes;
        return;
    }
    _prevFrameSize = frameSizeBytes;

    
    
    
    
    double deviation = DeviationFromExpectedDelay(frameDelayMS, deltaFS);

    if (abs(deviation) < _numStdDevDelayOutlier * sqrt(_varNoise) ||
        frameSizeBytes > _avgFrameSize + _numStdDevFrameSizeOutlier * sqrt(_varFrameSize))
    {
        
        
        EstimateRandomJitter(deviation, incompleteFrame);
        
        
        
        
        
        
        if ((!incompleteFrame || deviation >= 0.0) &&
            static_cast<double>(deltaFS) > - 0.25 * _maxFrameSize)
        {
            
            KalmanEstimateChannel(frameDelayMS, deltaFS);
        }
    }
    else
    {
        int nStdDev = (deviation >= 0) ? _numStdDevDelayOutlier : -_numStdDevDelayOutlier;
        EstimateRandomJitter(nStdDev * sqrt(_varNoise), incompleteFrame);
    }
    
    if (_startupCount >= kStartupDelaySamples)
    {
        PostProcessEstimate();
    }
    else
    {
        _startupCount++;
    }
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "Framesize statistics: max=%f average=%f", _maxFrameSize, _avgFrameSize);
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "The estimated slope is: theta=(%f, %f)", _theta[0], _theta[1]);
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "Random jitter: mean=%f variance=%f", _avgNoise, _varNoise);
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "Current jitter estimate: %f", _filterJitterEstimate);
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _receiverId),
               "Current max RTT: %u", _rttFilter.RttMs());
}


void
VCMJitterEstimator::FrameNacked()
{
    
    
    
    
    
    
    if (_nackCount < _nackLimit)
    {
        _nackCount++;
    }
}



void
VCMJitterEstimator::KalmanEstimateChannel(WebRtc_Word64 frameDelayMS,
                                          WebRtc_Word32 deltaFSBytes)
{
    double Mh[2];
    double hMh_sigma;
    double kalmanGain[2];
    double measureRes;
    double t00, t01;

    

    
    
    _thetaCov[0][0] += _Qcov[0][0];
    _thetaCov[0][1] += _Qcov[0][1];
    _thetaCov[1][0] += _Qcov[1][0];
    _thetaCov[1][1] += _Qcov[1][1];

    
    
    
    
    
    Mh[0] = _thetaCov[0][0] * deltaFSBytes + _thetaCov[0][1];
    Mh[1] = _thetaCov[1][0] * deltaFSBytes + _thetaCov[1][1];
    
    
    if (_maxFrameSize < 1.0)
    {
        return;
    }
    double sigma = (300.0 * exp(-abs(static_cast<double>(deltaFSBytes)) /
                   (1e0 * _maxFrameSize)) + 1) * sqrt(_varNoise);
    if (sigma < 1.0)
    {
        sigma = 1.0;
    }
    hMh_sigma = deltaFSBytes * Mh[0] + Mh[1] + sigma;
    if ((hMh_sigma < 1e-9 && hMh_sigma >= 0) || (hMh_sigma > -1e-9 && hMh_sigma <= 0))
    {
        assert(false);
        return;
    }
    kalmanGain[0] = Mh[0] / hMh_sigma;
    kalmanGain[1] = Mh[1] / hMh_sigma;

    
    
    measureRes = frameDelayMS - (deltaFSBytes * _theta[0] + _theta[1]);
    _theta[0] += kalmanGain[0] * measureRes;
    _theta[1] += kalmanGain[1] * measureRes;

    if (_theta[0] < _thetaLow)
    {
        _theta[0] = _thetaLow;
    }

    
    t00 = _thetaCov[0][0];
    t01 = _thetaCov[0][1];
    _thetaCov[0][0] = (1 - kalmanGain[0] * deltaFSBytes) * t00 -
                      kalmanGain[0] * _thetaCov[1][0];
    _thetaCov[0][1] = (1 - kalmanGain[0] * deltaFSBytes) * t01 -
                      kalmanGain[0] * _thetaCov[1][1];
    _thetaCov[1][0] = _thetaCov[1][0] * (1 - kalmanGain[1]) -
                      kalmanGain[1] * deltaFSBytes * t00;
    _thetaCov[1][1] = _thetaCov[1][1] * (1 - kalmanGain[1]) -
                      kalmanGain[1] * deltaFSBytes * t01;

    
    assert(_thetaCov[0][0] + _thetaCov[1][1] >= 0 &&
           _thetaCov[0][0] * _thetaCov[1][1] - _thetaCov[0][1] * _thetaCov[1][0] >= 0 &&
           _thetaCov[0][0] >= 0);
}



double
VCMJitterEstimator::DeviationFromExpectedDelay(WebRtc_Word64 frameDelayMS,
                                               WebRtc_Word32 deltaFSBytes) const
{
    return frameDelayMS - (_theta[0] * deltaFSBytes + _theta[1]);
}



void
VCMJitterEstimator::EstimateRandomJitter(double d_dT, bool incompleteFrame)
{
    double alpha;
    if (_alphaCount == 0)
    {
        assert(_alphaCount > 0);
        return;
    }
    alpha = static_cast<double>(_alphaCount - 1) / static_cast<double>(_alphaCount);
    _alphaCount++;
    if (_alphaCount > _alphaCountMax)
    {
        _alphaCount = _alphaCountMax;
    }
    double avgNoise = alpha * _avgNoise + (1 - alpha) * d_dT;
    double varNoise = alpha * _varNoise +
                      (1 - alpha) * (d_dT - _avgNoise) * (d_dT - _avgNoise);
    if (!incompleteFrame || varNoise > _varNoise)
    {
        _avgNoise = avgNoise;
        _varNoise = varNoise;
    }
    if (_varNoise < 1.0)
    {
        
        
        _varNoise = 1.0;
    }
}

double
VCMJitterEstimator::NoiseThreshold() const
{
    double noiseThreshold = _noiseStdDevs * sqrt(_varNoise) - _noiseStdDevOffset;
    if (noiseThreshold < 1.0)
    {
        noiseThreshold = 1.0;
    }
    return noiseThreshold;
}


double
VCMJitterEstimator::CalculateEstimate()
{
    double ret = _theta[0] * (_maxFrameSize - _avgFrameSize) + NoiseThreshold();

    
    if (ret < 1.0) {
        if (_prevEstimate <= 0.01)
        {
            ret = 1.0;
        }
        else
        {
            ret = _prevEstimate;
        }
    }
    if (ret > 10000.0) 
    {
        ret = 10000.0;
    }
    _prevEstimate = ret;
    return ret;
}

void
VCMJitterEstimator::PostProcessEstimate()
{
    _filterJitterEstimate = CalculateEstimate();
}

void
VCMJitterEstimator::UpdateRtt(WebRtc_UWord32 rttMs)
{
    _rttFilter.Update(rttMs);
}

void
VCMJitterEstimator::UpdateMaxFrameSize(WebRtc_UWord32 frameSizeBytes)
{
    if (_maxFrameSize < frameSizeBytes)
    {
        _maxFrameSize = frameSizeBytes;
    }
}



double
VCMJitterEstimator::GetJitterEstimate(double rttMultiplier)
{
    double jitterMS = CalculateEstimate();
    if (_filterJitterEstimate > jitterMS)
    {
        jitterMS = _filterJitterEstimate;
    }
    if (_nackCount >= _nackLimit)
    {
        return jitterMS + _rttFilter.RttMs() * rttMultiplier;
    }
    return jitterMS;
}

}
