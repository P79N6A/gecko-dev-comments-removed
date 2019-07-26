








#include "webrtc/test/video_renderer.h"

#include "webrtc/test/linux/glx_renderer.h"

namespace webrtc {
namespace test {

VideoRenderer* VideoRenderer::CreatePlatformRenderer(const char* window_title,
                                                     size_t width,
                                                     size_t height) {
  GlxRenderer* glx_renderer = GlxRenderer::Create(window_title, width, height);
  if (glx_renderer != NULL) {
    return glx_renderer;
  }
  return NULL;
}
}  
}  
