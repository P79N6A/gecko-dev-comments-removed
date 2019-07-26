












#ifndef VPM_FRAME_PREPROCESSOR_H
#define VPM_FRAME_PREPROCESSOR_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/modules/video_processing/main/source/content_analysis.h"
#include "webrtc/modules/video_processing/main/source/spatial_resampler.h"
#include "webrtc/modules/video_processing/main/source/video_decimator.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class VPMFramePreprocessor
{
public:

    VPMFramePreprocessor();
    ~VPMFramePreprocessor();

    int32_t ChangeUniqueId(const int32_t id);

    void Reset();

    
    void EnableTemporalDecimation(bool enable);

    void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);

    
    void EnableContentAnalysis(bool enable);

    
    int32_t SetMaxFrameRate(uint32_t maxFrameRate);

    
    int32_t SetTargetResolution(uint32_t width, uint32_t height,
                                uint32_t frameRate);

    
    void UpdateIncomingFrameRate();

    int32_t updateIncomingFrameSize(uint32_t width, uint32_t height);

    
    uint32_t DecimatedFrameRate();
    uint32_t DecimatedWidth() const;
    uint32_t DecimatedHeight() const;

    
    int32_t PreprocessFrame(const I420VideoFrame& frame,
                            I420VideoFrame** processedFrame);
    VideoContentMetrics* ContentMetrics() const;

private:
    
    
    enum { kSkipFrameCA = 2 };

    int32_t              _id;
    VideoContentMetrics*      _contentMetrics;
    uint32_t             _maxFrameRate;
    I420VideoFrame           _resampledFrame;
    VPMSpatialResampler*     _spatialResampler;
    VPMContentAnalysis*      _ca;
    VPMVideoDecimator*       _vd;
    bool                     _enableCA;
    int                      _frameCnt;
    
}; 

}  

#endif
