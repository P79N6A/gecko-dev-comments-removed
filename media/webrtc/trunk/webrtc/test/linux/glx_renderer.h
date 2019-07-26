









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_LINUX_GLX_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_LINUX_GLX_RENDERER_H_

#include <GL/glx.h>
#include <X11/Xlib.h>

#include "webrtc/test/gl/gl_renderer.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class GlxRenderer : public GlRenderer {
 public:
  static GlxRenderer* Create(const char* window_title, size_t width,
                             size_t height);
  virtual ~GlxRenderer();

  virtual void RenderFrame(const webrtc::I420VideoFrame& frame, int delta)
      OVERRIDE;
 private:
  GlxRenderer(size_t width, size_t height);

  bool Init(const char* window_title);
  void Resize(size_t width, size_t height);
  void Destroy();

  size_t width_, height_;

  Display* display_;
  Window window_;
  GLXContext context_;
};
}  
}  

#endif  
