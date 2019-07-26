









#ifndef VPM_CONTENT_ANALYSIS_H
#define VPM_CONTENT_ANALYSIS_H

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_processing/main/interface/video_processing_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMContentAnalysis
{
public:
    
    
    VPMContentAnalysis(bool runtime_cpu_detection);
    ~VPMContentAnalysis();

    
    
    
    
    int32_t Initialize(int width, int height);

    
    
    
    
    VideoContentMetrics* ComputeContentMetrics(const I420VideoFrame&
                                               inputFrame);

    
    
    int32_t Release();

private:

    
    VideoContentMetrics* ContentMetrics();

    
    typedef int32_t (VPMContentAnalysis::*TemporalDiffMetricFunc)();
    TemporalDiffMetricFunc TemporalDiffMetric;
    int32_t TemporalDiffMetric_C();

    
    int32_t ComputeMotionMetrics();

    
    
    typedef int32_t (VPMContentAnalysis::*ComputeSpatialMetricsFunc)();
    ComputeSpatialMetricsFunc ComputeSpatialMetrics;
    int32_t ComputeSpatialMetrics_C();

#if defined(WEBRTC_ARCH_X86_FAMILY)
    int32_t ComputeSpatialMetrics_SSE2();
    int32_t TemporalDiffMetric_SSE2();
#endif

    const uint8_t*       _origFrame;
    uint8_t*             _prevFrame;
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
