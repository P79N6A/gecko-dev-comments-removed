









#ifndef WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H
#define WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/modules/video_processing/main/source/brighten.h"
#include "webrtc/modules/video_processing/main/source/brightness_detection.h"
#include "webrtc/modules/video_processing/main/source/color_enhancement.h"
#include "webrtc/modules/video_processing/main/source/deflickering.h"
#include "webrtc/modules/video_processing/main/source/frame_preprocessor.h"

namespace webrtc {
class CriticalSectionWrapper;

class VideoProcessingModuleImpl : public VideoProcessingModule {
 public:
  VideoProcessingModuleImpl(int32_t id);

  virtual ~VideoProcessingModuleImpl();

  int32_t Id() const;

  virtual int32_t ChangeUniqueId(const int32_t id) OVERRIDE;

  virtual void Reset() OVERRIDE;

  virtual int32_t Deflickering(I420VideoFrame* frame,
                               FrameStats* stats) OVERRIDE;

  virtual int32_t BrightnessDetection(const I420VideoFrame& frame,
                                      const FrameStats& stats) OVERRIDE;

  

  
  virtual void EnableTemporalDecimation(bool enable) OVERRIDE;

  virtual void SetInputFrameResampleMode(
      VideoFrameResampling resampling_mode) OVERRIDE;

  
  virtual void EnableContentAnalysis(bool enable) OVERRIDE;

  
  virtual int32_t SetTargetResolution(uint32_t width,
                                      uint32_t height,
                                      uint32_t frame_rate) OVERRIDE;


  
  virtual uint32_t Decimatedframe_rate() OVERRIDE;
  virtual uint32_t DecimatedWidth() const OVERRIDE;
  virtual uint32_t DecimatedHeight() const OVERRIDE;

  
  
  
  
  virtual int32_t PreprocessFrame(const I420VideoFrame& frame,
                                  I420VideoFrame** processed_frame) OVERRIDE;
  virtual VideoContentMetrics* ContentMetrics() const OVERRIDE;

 private:
  int32_t  id_;
  CriticalSectionWrapper& mutex_;
  VPMDeflickering deflickering_;
  VPMBrightnessDetection brightness_detection_;
  VPMFramePreprocessor  frame_pre_processor_;
};

}  

#endif
