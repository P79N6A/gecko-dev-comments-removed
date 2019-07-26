









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_  
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_

#include "webrtc/modules/video_render/include/video_render.h"
#include "system_wrappers/interface/list_wrapper.h"

namespace webrtc {


class VideoRenderFrames {
 public:
  VideoRenderFrames();
  ~VideoRenderFrames();

  
  WebRtc_Word32 AddFrame(I420VideoFrame* new_frame);

  
  I420VideoFrame* FrameToRender();

  
  WebRtc_Word32 ReturnFrame(I420VideoFrame* old_frame);

  
  WebRtc_Word32 ReleaseAllFrames();

  
  WebRtc_UWord32 TimeToNextFrameRelease();

  
  WebRtc_Word32 SetRenderDelay(const WebRtc_UWord32 render_delay);

 private:
  
  enum { KMaxNumberOfFrames = 300 };
  
  enum { KOldRenderTimestampMS = 500 };
  
  enum { KFutureRenderTimestampMS = 10000 };

  
  ListWrapper incoming_frames_;
  
  ListWrapper empty_frames_;

  
  WebRtc_UWord32 render_delay_ms_;
};

}  

#endif
