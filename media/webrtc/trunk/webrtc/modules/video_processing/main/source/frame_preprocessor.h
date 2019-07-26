












#ifndef VPM_FRAME_PREPROCESSOR_H
#define VPM_FRAME_PREPROCESSOR_H

#include "typedefs.h"
#include "video_processing.h"
#include "content_analysis.h"
#include "spatial_resampler.h"
#include "video_decimator.h"

namespace webrtc {


class VPMFramePreprocessor
{
public:

    VPMFramePreprocessor();
    ~VPMFramePreprocessor();

    WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    void Reset();

    
    void EnableTemporalDecimation(bool enable);

    void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);

    
    void EnableContentAnalysis(bool enable);

    
    WebRtc_Word32 SetMaxFrameRate(WebRtc_UWord32 maxFrameRate);

    
    WebRtc_Word32 SetTargetResolution(WebRtc_UWord32 width,
                                      WebRtc_UWord32 height,
                                      WebRtc_UWord32 frameRate);

    
    void UpdateIncomingFrameRate();

    WebRtc_Word32 updateIncomingFrameSize(WebRtc_UWord32 width,
                                          WebRtc_UWord32 height);

    
    WebRtc_UWord32 DecimatedFrameRate();
    WebRtc_UWord32 DecimatedWidth() const;
    WebRtc_UWord32 DecimatedHeight() const;

    
    WebRtc_Word32 PreprocessFrame(const I420VideoFrame& frame,
                                  I420VideoFrame** processedFrame);
    VideoContentMetrics* ContentMetrics() const;

private:
    
    
    enum { kSkipFrameCA = 2 };

    WebRtc_Word32              _id;
    VideoContentMetrics*      _contentMetrics;
    WebRtc_UWord32             _maxFrameRate;
    I420VideoFrame           _resampledFrame;
    VPMSpatialResampler*     _spatialResampler;
    VPMContentAnalysis*      _ca;
    VPMVideoDecimator*       _vd;
    bool                     _enableCA;
    int                      _frameCnt;
    
}; 

} 

#endif
