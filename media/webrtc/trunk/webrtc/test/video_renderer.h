








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_RENDERER_H_

#include <stddef.h>

#include "webrtc/video_renderer.h"

namespace webrtc {
namespace test {
class VideoRenderer : public webrtc::VideoRenderer {
 public:
  
  
  static VideoRenderer* Create(const char* window_title, size_t width,
                               size_t height);
  
  
  
  
  
  static VideoRenderer* CreatePlatformRenderer(const char* window_title,
                                               size_t width, size_t height);
  virtual ~VideoRenderer() {}
 protected:
  VideoRenderer() {}
};
}  
}  

#endif  
