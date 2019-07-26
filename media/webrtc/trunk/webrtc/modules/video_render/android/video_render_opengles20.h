









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_ANDROID_VIDEO_RENDER_OPENGLES20_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_ANDROID_VIDEO_RENDER_OPENGLES20_H_

#include "webrtc/modules/video_render/include/video_render_defines.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace webrtc
{

class VideoRenderOpenGles20 {
 public:
  VideoRenderOpenGles20(int32_t id);
  ~VideoRenderOpenGles20();

  int32_t Setup(int32_t widht, int32_t height);
  int32_t Render(const I420VideoFrame& frameToRender);
  int32_t SetCoordinates(int32_t zOrder, const float left, const float top,
                         const float right, const float bottom);

 private:
  void printGLString(const char *name, GLenum s);
  void checkGlError(const char* op);
  GLuint loadShader(GLenum shaderType, const char* pSource);
  GLuint createProgram(const char* pVertexSource,
                       const char* pFragmentSource);
  void SetupTextures(const I420VideoFrame& frameToRender);
  void UpdateTextures(const I420VideoFrame& frameToRender);

  int32_t _id;
  GLuint _textureIds[3]; 
  GLuint _program;
  GLsizei _textureWidth;
  GLsizei _textureHeight;

  GLfloat _vertices[20];
  static const char g_indices[];

  static const char g_vertextShader[];
  static const char g_fragmentShader[];

};

}  

#endif  
