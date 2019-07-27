









#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_CONTENT_ANALYSIS_H
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_CONTENT_ANALYSIS_H

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_processing/main/interface/video_processing_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMContentAnalysis {
 public:
  
  
  explicit VPMContentAnalysis(bool runtime_cpu_detection);
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

  const uint8_t* orig_frame_;
  scoped_ptr<uint8_t> prev_frame_;
  int width_;
  int height_;
  int skip_num_;
  int border_;

  
  float motion_magnitude_;   
  float spatial_pred_err_;   
  float spatial_pred_err_h_;  
  float spatial_pred_err_v_;  
  bool first_frame_;
  bool ca_Init_;

  scoped_ptr<VideoContentMetrics> content_metrics_;
};

}  

#endif  
