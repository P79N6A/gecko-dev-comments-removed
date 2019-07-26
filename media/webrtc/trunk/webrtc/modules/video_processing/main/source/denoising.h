









#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_DENOISING_H_
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_SOURCE_DENOISING_H_

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMDenoising {
 public:
  VPMDenoising();
  ~VPMDenoising();

  int32_t ChangeUniqueId(int32_t id);

  void Reset();

  int32_t ProcessFrame(I420VideoFrame* frame);

 private:
  int32_t id_;

  uint32_t* moment1_;  
  uint32_t* moment2_;  
  uint32_t  frame_size_;  
  int denoise_frame_cnt_;  
};

}  

#endif 

