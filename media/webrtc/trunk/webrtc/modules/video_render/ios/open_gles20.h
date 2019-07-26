









#ifndef WEBRTC_MODULES_VIDEO_RENDER_IOS_OPEN_GLES20_H_
#define WEBRTC_MODULES_VIDEO_RENDER_IOS_OPEN_GLES20_H_

#include <OpenGLES/ES2/glext.h>

#include "webrtc/modules/video_render/include/video_render_defines.h"





namespace webrtc {
class OpenGles20 {
 public:
  OpenGles20();
  ~OpenGles20();

  bool Setup(int32_t width, int32_t height);
  bool Render(const I420VideoFrame& frame);

  
  
  
  bool SetCoordinates(const float z_order,
                      const float left,
                      const float top,
                      const float right,
                      const float bottom);

 private:
  
  
  GLuint LoadShader(GLenum shader_type, const char* shader_source);

  GLuint CreateProgram(const char* vertex_source, const char* fragment_source);

  
  void SetupTextures(const I420VideoFrame& frame);

  
  void UpdateTextures(const I420VideoFrame& frame);

  GLuint texture_ids_[3];  
  GLuint program_;
  GLsizei texture_width_;
  GLsizei texture_height_;

  GLfloat vertices_[20];
  static const char indices_[];
  static const char vertext_shader_[];
  static const char fragment_shader_[];
};
}  
#endif  
