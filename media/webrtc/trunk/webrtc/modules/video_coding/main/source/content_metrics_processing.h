









#ifndef WEBRTC_MODULES_VIDEO_CODING_CONTENT_METRICS_PROCESSING_H_
#define WEBRTC_MODULES_VIDEO_CODING_CONTENT_METRICS_PROCESSING_H_

#include "webrtc/typedefs.h"

namespace webrtc {

struct VideoContentMetrics;


enum {
  kQmMinIntervalMs = 10000
};


enum {
  kNfdMetric = 1
};




class VCMContentMetricsProcessing {
 public:
  VCMContentMetricsProcessing();
  ~VCMContentMetricsProcessing();

  
  int UpdateContentData(const VideoContentMetrics *contentMetrics);

  
  void ResetShortTermAvgData();

  
  int Reset();

  
  void UpdateFrameRate(float frameRate);

  
  
  VideoContentMetrics* LongTermAvgData();

  
  
  VideoContentMetrics* ShortTermAvgData();

 private:
  
  int ProcessContent(const VideoContentMetrics *contentMetrics);

  
  void UpdateRecursiveAvg(const VideoContentMetrics *contentMetrics);

  
  void UpdateUniformAvg(const VideoContentMetrics *contentMetrics);

  VideoContentMetrics* recursive_avg_;
  VideoContentMetrics* uniform_avg_;
  float recursive_avg_factor_;
  uint32_t frame_cnt_uniform_avg_;
  float avg_motion_level_;
  float avg_spatial_level_;
};
}  
#endif  
