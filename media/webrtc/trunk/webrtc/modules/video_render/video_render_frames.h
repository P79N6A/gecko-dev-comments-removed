









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_  
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_

#include <list>

#include "webrtc/modules/video_render/include/video_render.h"

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
  typedef std::list<I420VideoFrame*> FrameList;

  
  enum { KMaxNumberOfFrames = 300 };
  
  enum { KOldRenderTimestampMS = 500 };
  
  enum { KFutureRenderTimestampMS = 10000 };

  
  FrameList incoming_frames_;
  
  FrameList empty_frames_;

  
  uint32_t render_delay_ms_;
};

}  

#endif  
