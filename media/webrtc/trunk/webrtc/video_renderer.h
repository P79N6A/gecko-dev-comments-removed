









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RENDERER_H_

namespace webrtc {

class I420VideoFrame;

class VideoRenderer {
 public:
  
  
  
  virtual void RenderFrame(const I420VideoFrame& video_frame,
                           int time_to_render_ms) = 0;

 protected:
  virtual ~VideoRenderer() {}
};
}  

#endif  
