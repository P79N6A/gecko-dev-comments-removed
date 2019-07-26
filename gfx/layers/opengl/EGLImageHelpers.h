





#ifndef EGLIMAGEHELPERS_H_
#define EGLIMAGEHELPERS_H_

typedef void* EGLImage;

namespace mozilla {
namespace gl {
    class GLContext;
}

namespace layers {

EGLImage EGLImageCreateFromNativeBuffer(gl::GLContext* aGL, void* aBuffer);
void EGLImageDestroy(gl::GLContext* aGL, EGLImage aImage);

} 
} 

#endif 
