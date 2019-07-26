












#ifndef MODULES_VIDEO_PROCESSING_MAIN_SOURCE_BRIGHTNESS_DETECTION_H
#define MODULES_VIDEO_PROCESSING_MAIN_SOURCE_BRIGHTNESS_DETECTION_H
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMBrightnessDetection {
 public:
  VPMBrightnessDetection();
  ~VPMBrightnessDetection();
  int32_t ChangeUniqueId(int32_t id);

  void Reset();
  int32_t ProcessFrame(const I420VideoFrame& frame,
                       const VideoProcessingModule::FrameStats& stats);

 private:
  int32_t id_;

  uint32_t frame_cnt_bright_;
  uint32_t frame_cnt_dark_;
};

}  

#endif 
