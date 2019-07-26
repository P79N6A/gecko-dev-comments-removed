









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCOPED_PIXEL_BUFFER_OBJECT_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCOPED_PIXEL_BUFFER_OBJECT_H_

#include <OpenGL/CGLMacro.h>
#include <OpenGL/OpenGL.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class ScopedPixelBufferObject {
 public:
  ScopedPixelBufferObject();
  ~ScopedPixelBufferObject();

  bool Init(CGLContextObj cgl_context, int size_in_bytes);
  void Release();

  GLuint get() const { return pixel_buffer_object_; }

 private:
  CGLContextObj cgl_context_;
  GLuint pixel_buffer_object_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPixelBufferObject);
};

}  

#endif 
