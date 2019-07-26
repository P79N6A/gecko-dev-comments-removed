









#ifndef WEBRTC_MODULES_VIDEO_RENDER_IOS_VIDEO_RENDER_IOS_CHANNEL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_IOS_VIDEO_RENDER_IOS_CHANNEL_H_

#include "webrtc/modules/video_render/include/video_render_defines.h"
#include "webrtc/modules/video_render/ios/video_render_ios_view.h"

namespace webrtc {

class VideoRenderIosGles20;

class VideoRenderIosChannel : public VideoRenderCallback {
 public:
  explicit VideoRenderIosChannel(VideoRenderIosView* view);
  virtual ~VideoRenderIosChannel();

  
  virtual int32_t RenderFrame(const uint32_t stream_id,
                              I420VideoFrame& video_frame) OVERRIDE;

  int SetStreamSettings(const float z_order,
                        const float left,
                        const float top,
                        const float right,
                        const float bottom);
  bool IsUpdated();
  bool RenderOffScreenBuffer();

 private:
  VideoRenderIosView* view_;
  I420VideoFrame* current_frame_;
  bool buffer_is_updated_;
};

}  
#endif  
