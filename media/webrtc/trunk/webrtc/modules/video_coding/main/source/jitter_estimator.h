









#ifndef WEBRTC_MODULES_VIDEO_CODING_JITTER_ESTIMATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_JITTER_ESTIMATOR_H_

#include "typedefs.h"
#include "rtt_filter.h"

namespace webrtc
{

class VCMJitterEstimator
{
public:
    VCMJitterEstimator(WebRtc_Word32 vcmId = 0, WebRtc_Word32 receiverId = 0);

    VCMJitterEstimator& operator=(const VCMJitterEstimator& rhs);

    
    void Reset();
    void ResetNackCount();

    
    
    
    
    
    
    
    void UpdateEstimate(WebRtc_Word64 frameDelayMS,
                        WebRtc_UWord32 frameSizeBytes,
                        bool incompleteFrame = false);

    
    
    
    
    
    
    double GetJitterEstimate(double rttMultiplier);

    
    void FrameNacked();

    
    
    
    
    void UpdateRtt(WebRtc_UWord32 rttMs);

    void UpdateMaxFrameSize(WebRtc_UWord32 frameSizeBytes);

    
    
    
    static const WebRtc_UWord32 OPERATING_SYSTEM_JITTER = 10;

protected:
    
    double              _theta[2]; 
    double              _varNoise; 

private:
    
    
    
    
    
    
    
    void KalmanEstimateChannel(WebRtc_Word64 frameDelayMS, WebRtc_Word32 deltaFSBytes);

    
    
    
    
    
    
    
    void EstimateRandomJitter(double d_dT, bool incompleteFrame);

    double NoiseThreshold() const;

    
    
    
    double CalculateEstimate();

    
    void PostProcessEstimate();

    
    
    
    
    
    
    
    
    
    double DeviationFromExpectedDelay(WebRtc_Word64 frameDelayMS,
                                      WebRtc_Word32 deltaFSBytes) const;

    
    WebRtc_Word32         _vcmId;
    WebRtc_Word32         _receiverId;
    const double          _phi;
    const double          _psi;
    const WebRtc_UWord32  _alphaCountMax;
    const double          _thetaLow;
    const WebRtc_UWord32  _nackLimit;
    const WebRtc_Word32   _numStdDevDelayOutlier;
    const WebRtc_Word32   _numStdDevFrameSizeOutlier;
    const double          _noiseStdDevs;
    const double          _noiseStdDevOffset;

    double                _thetaCov[2][2]; 
    double                _Qcov[2][2];     
    double                _avgFrameSize;   
    double                _varFrameSize;   
    double                _maxFrameSize;   
                                           
    WebRtc_UWord32        _fsSum;
    WebRtc_UWord32        _fsCount;

    WebRtc_Word64         _lastUpdateT;
    double                _prevEstimate;         
    WebRtc_UWord32        _prevFrameSize;        
    double                _avgNoise;             
    WebRtc_UWord32        _alphaCount;
    double                _filterJitterEstimate; 

    WebRtc_UWord32        _startupCount;

    WebRtc_Word64         _latestNackTimestamp;  
    WebRtc_UWord32        _nackCount;            
                                                 
    VCMRttFilter          _rttFilter;

    enum { kStartupDelaySamples = 30 };
    enum { kFsAccuStartupSamples = 5 };
};

} 

#endif 
