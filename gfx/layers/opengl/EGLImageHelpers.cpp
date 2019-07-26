





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

    GLLibraryEGL* egl = aGL->GetLibraryEGL();
    if (!egl) {
        NS_WARNING("Failed to obtain pointer to EGL. Returning EGL_NO_IMAGE.");
        return EGL_NO_IMAGE;
    }

    return egl->fCreateImage(egl->Display(),
                             EGL_NO_CONTEXT,
                             LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                             aBuffer, attrs);
}

void
EGLImageDestroy(GLContext* aGL, EGLImage aImage)
{
    GLLibraryEGL* egl = aGL->GetLibraryEGL();
    if (!egl) {
        NS_WARNING("Failed to obtain pointer to EGL. Image not destroyed.");
        return;
    }

    egl->fDestroyImage(egl->Display(), aImage);
}

} 
} 
