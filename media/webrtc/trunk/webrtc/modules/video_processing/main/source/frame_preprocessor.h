












#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_FRAME_PREPROCESSOR_H
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_FRAME_PREPROCESSOR_H

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/modules/video_processing/main/source/content_analysis.h"
#include "webrtc/modules/video_processing/main/source/spatial_resampler.h"
#include "webrtc/modules/video_processing/main/source/video_decimator.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMFramePreprocessor {
 public:
  VPMFramePreprocessor();
  ~VPMFramePreprocessor();

  int32_t ChangeUniqueId(const int32_t id);

  void Reset();

  
  void EnableTemporalDecimation(bool enable);

  void SetInputFrameResampleMode(VideoFrameResampling resampling_mode);

  
  void EnableContentAnalysis(bool enable);

  
  int32_t SetMaxFramerate(uint32_t max_frame_rate);

  
  int32_t SetTargetResolution(uint32_t width, uint32_t height,
                              uint32_t frame_rate);

  
  void UpdateIncomingframe_rate();

  int32_t updateIncomingFrameSize(uint32_t width, uint32_t height);

  
  uint32_t Decimatedframe_rate();
  uint32_t DecimatedWidth() const;
  uint32_t DecimatedHeight() const;

  
  int32_t PreprocessFrame(const I420VideoFrame& frame,
                          I420VideoFrame** processed_frame);
  VideoContentMetrics* ContentMetrics() const;

 private:
  
  
  enum { kSkipFrameCA = 2 };

  int32_t id_;
  VideoContentMetrics* content_metrics_;
  uint32_t max_frame_rate_;
  I420VideoFrame resampled_frame_;
  VPMSpatialResampler* spatial_resampler_;
  VPMContentAnalysis* ca_;
  VPMVideoDecimator* vd_;
  bool enable_ca_;
  int frame_cnt_;

};

}  

#endif
