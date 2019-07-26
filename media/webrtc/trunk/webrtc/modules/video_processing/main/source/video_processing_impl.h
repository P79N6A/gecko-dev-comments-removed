









#ifndef WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H
#define WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/modules/video_processing/main/source/brighten.h"
#include "webrtc/modules/video_processing/main/source/brightness_detection.h"
#include "webrtc/modules/video_processing/main/source/color_enhancement.h"
#include "webrtc/modules/video_processing/main/source/deflickering.h"
#include "webrtc/modules/video_processing/main/source/denoising.h"
#include "webrtc/modules/video_processing/main/source/frame_preprocessor.h"

namespace webrtc {
class CriticalSectionWrapper;

class VideoProcessingModuleImpl : public VideoProcessingModule
{
public:

    VideoProcessingModuleImpl(int32_t id);

    virtual ~VideoProcessingModuleImpl();

    int32_t Id() const;

    virtual int32_t ChangeUniqueId(const int32_t id);

    virtual void Reset();

    virtual int32_t Deflickering(I420VideoFrame* frame, FrameStats* stats);

    virtual int32_t Denoising(I420VideoFrame* frame);

    virtual int32_t BrightnessDetection(const I420VideoFrame& frame,
                                        const FrameStats& stats);

    

    
    virtual void EnableTemporalDecimation(bool enable);

    virtual void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);

    
    virtual void EnableContentAnalysis(bool enable);

    
    virtual int32_t SetMaxFrameRate(uint32_t maxFrameRate);

    
    virtual int32_t SetTargetResolution(uint32_t width,
                                        uint32_t height,
                                        uint32_t frameRate);


    
    virtual uint32_t DecimatedFrameRate();
    virtual uint32_t DecimatedWidth() const;
    virtual uint32_t DecimatedHeight() const;

    
    
    
    
    virtual int32_t PreprocessFrame(const I420VideoFrame& frame,
                                    I420VideoFrame** processedFrame);
    virtual VideoContentMetrics* ContentMetrics() const;

private:
    int32_t              _id;
    CriticalSectionWrapper&    _mutex;

    VPMDeflickering            _deflickering;
    VPMDenoising               _denoising;
    VPMBrightnessDetection     _brightnessDetection;
    VPMFramePreprocessor       _framePreProcessor;
};

}  

#endif
