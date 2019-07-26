









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_GL_GL_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_GL_GL_RENDERER_H_

#ifdef WEBRTC_MAC
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "webrtc/test/video_renderer.h"
#include "webrtc/typedefs.h"


namespace webrtc {
namespace test {

class GlRenderer : public VideoRenderer {
 public:
  virtual void RenderFrame(const webrtc::I420VideoFrame& frame,
                           int time_to_render_ms) OVERRIDE;

 protected:
  GlRenderer();

  void Init();
  void Destroy();

  void ResizeViewport(size_t width, size_t height);

 private:
  bool is_init_;
  uint8_t* buffer_;
  GLuint texture_;
  size_t width_, height_, buffer_size_;

  void ResizeVideo(size_t width, size_t height);
};
}  
}  

#endif  
