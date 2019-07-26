









#ifndef WEBRTC_MODULES_VIDEO_CODING_JITTER_ESTIMATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_JITTER_ESTIMATOR_H_

#include "webrtc/modules/video_coding/main/source/rtt_filter.h"
#include "webrtc/typedefs.h"

namespace webrtc
{

class VCMJitterEstimator
{
public:
    VCMJitterEstimator(int32_t vcmId = 0, int32_t receiverId = 0);

    VCMJitterEstimator& operator=(const VCMJitterEstimator& rhs);

    
    void Reset();
    void ResetNackCount();

    
    
    
    
    
    
    
    void UpdateEstimate(int64_t frameDelayMS,
                        uint32_t frameSizeBytes,
                        bool incompleteFrame = false);

    
    
    
    
    
    
    int GetJitterEstimate(double rttMultiplier);

    
    void FrameNacked();

    
    
    
    
    void UpdateRtt(uint32_t rttMs);

    void UpdateMaxFrameSize(uint32_t frameSizeBytes);

    
    
    
    static const uint32_t OPERATING_SYSTEM_JITTER = 10;

protected:
    
    double              _theta[2]; 
    double              _varNoise; 

private:
    
    
    
    
    
    
    
    void KalmanEstimateChannel(int64_t frameDelayMS, int32_t deltaFSBytes);

    
    
    
    
    
    
    
    void EstimateRandomJitter(double d_dT, bool incompleteFrame);

    double NoiseThreshold() const;

    
    
    
    double CalculateEstimate();

    
    void PostProcessEstimate();

    
    
    
    
    
    
    
    
    
    double DeviationFromExpectedDelay(int64_t frameDelayMS,
                                      int32_t deltaFSBytes) const;

    
    int32_t         _vcmId;
    int32_t         _receiverId;
    const double          _phi;
    const double          _psi;
    const uint32_t  _alphaCountMax;
    const double          _thetaLow;
    const uint32_t  _nackLimit;
    const int32_t   _numStdDevDelayOutlier;
    const int32_t   _numStdDevFrameSizeOutlier;
    const double          _noiseStdDevs;
    const double          _noiseStdDevOffset;

    double                _thetaCov[2][2]; 
    double                _Qcov[2][2];     
    double                _avgFrameSize;   
    double                _varFrameSize;   
    double                _maxFrameSize;   
                                           
    uint32_t        _fsSum;
    uint32_t        _fsCount;

    int64_t         _lastUpdateT;
    double                _prevEstimate;         
    uint32_t        _prevFrameSize;        
    double                _avgNoise;             
    uint32_t        _alphaCount;
    double                _filterJitterEstimate; 

    uint32_t        _startupCount;

    int64_t         _latestNackTimestamp;  
    uint32_t        _nackCount;            
                                                 
    VCMRttFilter          _rttFilter;

    enum { kStartupDelaySamples = 30 };
    enum { kFsAccuStartupSamples = 5 };
};

}  

#endif 
