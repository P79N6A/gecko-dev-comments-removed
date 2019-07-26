








#include "webrtc/test/video_renderer.h"



#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class NullRenderer : public VideoRenderer {
  virtual void RenderFrame(const I420VideoFrame& video_frame,
                           int time_to_render_ms) OVERRIDE {}
};

VideoRenderer* VideoRenderer::Create(const char* window_title,
                                     size_t width,
                                     size_t height) {
  VideoRenderer* renderer = CreatePlatformRenderer(window_title, width, height);
  if (renderer != NULL) {
    
    return renderer;
  }

  return new NullRenderer();
}
}  
}  
