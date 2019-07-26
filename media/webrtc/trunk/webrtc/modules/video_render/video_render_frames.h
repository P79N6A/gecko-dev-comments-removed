









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_  
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_

#include "webrtc/modules/video_render/include/video_render.h"
#include "webrtc/system_wrappers/interface/list_wrapper.h"

namespace webrtc {


class VideoRenderFrames {
 public:
  VideoRenderFrames();
  ~VideoRenderFrames();

  
  int32_t AddFrame(I420VideoFrame* new_frame);

  
  I420VideoFrame* FrameToRender();

  
  int32_t ReturnFrame(I420VideoFrame* old_frame);

  
  int32_t ReleaseAllFrames();

  
  uint32_t TimeToNextFrameRelease();

  
  int32_t SetRenderDelay(const uint32_t render_delay);

 private:
  
  enum { KMaxNumberOfFrames = 300 };
  
  enum { KOldRenderTimestampMS = 500 };
  
  enum { KFutureRenderTimestampMS = 10000 };

  
  ListWrapper incoming_frames_;
  
  ListWrapper empty_frames_;

  
  uint32_t render_delay_ms_;
};

}  

#endif
