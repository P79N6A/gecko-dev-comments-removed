





#include "EGLImageHelpers.h"
#include "GLContext.h"
#include "GLLibraryEGL.h"

namespace mozilla
{
namespace layers {

using namespace gl;

EGLImage
EGLImageCreateFromNativeBuffer(GLContext* aGL, void* aBuffer)
{
    EGLint attrs[] = {
        LOCAL_EGL_IMAGE_PRESERVED, LOCAL_EGL_TRUE,
        LOCAL_EGL_NONE, LOCAL_EGL_NONE
    };

    return sEGLLibrary.fCreateImage(sEGLLibrary.Display(),
                                     EGL_NO_CONTEXT,
                                     LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                                     aBuffer, attrs);
}

void
EGLImageDestroy(GLContext* aGL, EGLImage aImage)
{
    sEGLLibrary.fDestroyImage(sEGLLibrary.Display(), aImage);
}

} 
} 
