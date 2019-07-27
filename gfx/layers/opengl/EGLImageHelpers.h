





#ifndef EGLIMAGEHELPERS_H_
#define EGLIMAGEHELPERS_H_

#include "mozilla/gfx/Point.h"

typedef void* EGLImage;

namespace mozilla {
namespace gl {
    class GLContext;
}

namespace layers {

EGLImage EGLImageCreateFromNativeBuffer(gl::GLContext* aGL, void* aBuffer, const gfx::IntSize& aCropSize);
void EGLImageDestroy(gl::GLContext* aGL, EGLImage aImage);

} 
} 

#endif 
