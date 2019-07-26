









#ifndef VPM_CONTENT_ANALYSIS_H
#define VPM_CONTENT_ANALYSIS_H

#include "common_video/interface/i420_video_frame.h"
#include "typedefs.h"
#include "module_common_types.h"
#include "video_processing_defines.h"

namespace webrtc {

class VPMContentAnalysis
{
public:
    
    
    VPMContentAnalysis(bool runtime_cpu_detection);
    ~VPMContentAnalysis();

    
    
    
    
    WebRtc_Word32 Initialize(int width, int height);

    
    
    
    
    VideoContentMetrics* ComputeContentMetrics(const I420VideoFrame&
                                               inputFrame);

    
    
    WebRtc_Word32 Release();

private:

    
    VideoContentMetrics* ContentMetrics();

    
    typedef WebRtc_Word32 (VPMContentAnalysis::*TemporalDiffMetricFunc)();
    TemporalDiffMetricFunc TemporalDiffMetric;
    WebRtc_Word32 TemporalDiffMetric_C();

    
    WebRtc_Word32 ComputeMotionMetrics();

    
    
    typedef WebRtc_Word32 (VPMContentAnalysis::*ComputeSpatialMetricsFunc)();
    ComputeSpatialMetricsFunc ComputeSpatialMetrics;
    WebRtc_Word32 ComputeSpatialMetrics_C();

#if defined(WEBRTC_ARCH_X86_FAMILY)
    WebRtc_Word32 ComputeSpatialMetrics_SSE2();
    WebRtc_Word32 TemporalDiffMetric_SSE2();
#endif

    const WebRtc_UWord8*       _origFrame;
    WebRtc_UWord8*             _prevFrame;
    int                        _width;
    int                        _height;
    int                        _skipNum;
    int                        _border;

    
    
    float                  _motionMagnitude;    
    float                  _spatialPredErr;     
    float                  _spatialPredErrH;    
    float                  _spatialPredErrV;    
    bool                   _firstFrame;
    bool                   _CAInit;

    VideoContentMetrics*   _cMetrics;

}; 

} 

#endif
