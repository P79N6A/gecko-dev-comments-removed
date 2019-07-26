









#ifndef WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H
#define WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H

#include "video_processing.h"
#include "brighten.h"
#include "brightness_detection.h"
#include "color_enhancement.h"
#include "deflickering.h"
#include "denoising.h"
#include "frame_preprocessor.h"

namespace webrtc {
class CriticalSectionWrapper;

class VideoProcessingModuleImpl : public VideoProcessingModule
{
public:

    VideoProcessingModuleImpl(WebRtc_Word32 id);

    virtual ~VideoProcessingModuleImpl();

    WebRtc_Word32 Id() const;

    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    virtual void Reset();

    virtual WebRtc_Word32 Deflickering(I420VideoFrame* frame,
                                       FrameStats* stats);

    virtual WebRtc_Word32 Denoising(I420VideoFrame* frame);

    virtual WebRtc_Word32 BrightnessDetection(const I420VideoFrame& frame,
                                              const FrameStats& stats);

    

    
    virtual void EnableTemporalDecimation(bool enable);

    virtual void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);

    
    virtual void EnableContentAnalysis(bool enable);

    
    virtual WebRtc_Word32 SetMaxFrameRate(WebRtc_UWord32 maxFrameRate);

    
    virtual WebRtc_Word32 SetTargetResolution(WebRtc_UWord32 width,
                                              WebRtc_UWord32 height,
                                              WebRtc_UWord32 frameRate);


    
    virtual WebRtc_UWord32 DecimatedFrameRate();
    virtual WebRtc_UWord32 DecimatedWidth() const;
    virtual WebRtc_UWord32 DecimatedHeight() const;

    
    
    
    
    virtual WebRtc_Word32 PreprocessFrame(const I420VideoFrame& frame,
                                          I420VideoFrame** processedFrame);
    virtual VideoContentMetrics* ContentMetrics() const;

private:
    WebRtc_Word32              _id;
    CriticalSectionWrapper&    _mutex;

    VPMDeflickering            _deflickering;
    VPMDenoising               _denoising;
    VPMBrightnessDetection     _brightnessDetection;
    VPMFramePreprocessor       _framePreProcessor;
};

} 

#endif
