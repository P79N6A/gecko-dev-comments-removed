



#include "EGLUtils.h"

#include "GLContextEGL.h"

namespace mozilla {
namespace gl {

bool
DoesEGLContextSupportSharingWithEGLImage(GLContext* gl)
{
    return sEGLLibrary.HasKHRImageBase() &&
           sEGLLibrary.HasKHRImageTexture2D() &&
           gl->IsExtensionSupported(GLContext::OES_EGL_image);
}

EGLImage
CreateEGLImage(GLContext* gl, GLuint tex)
{
    MOZ_ASSERT(DoesEGLContextSupportSharingWithEGLImage(gl));

    EGLContext eglContext = GLContextEGL::Cast(gl)->GetEGLContext();
    EGLImage image = sEGLLibrary.fCreateImage(EGL_DISPLAY(),
                                              eglContext,
                                              LOCAL_EGL_GL_TEXTURE_2D,
                                              (EGLClientBuffer)tex,
                                              nullptr);
    return image;
}




 EGLImageWrapper*
EGLImageWrapper::Create(GLContext* gl, GLuint tex)
{
    MOZ_ASSERT(DoesEGLContextSupportSharingWithEGLImage(gl));

    GLLibraryEGL& library = sEGLLibrary;
    EGLDisplay display = EGL_DISPLAY();
    EGLContext eglContext = GLContextEGL::Cast(gl)->GetEGLContext();
    EGLImage image = library.fCreateImage(display,
                                          eglContext,
                                          LOCAL_EGL_GL_TEXTURE_2D,
                                          (EGLClientBuffer)tex,
                                          nullptr);
    if (!image) {
#ifdef DEBUG
        printf_stderr("Could not create EGL images: ERROR (0x%04x)\n",
                      sEGLLibrary.fGetError());
#endif
        return nullptr;
    }

    return new EGLImageWrapper(library, display, image);
}

EGLImageWrapper::~EGLImageWrapper()
{
    mLibrary.fDestroyImage(mDisplay, mImage);
}

bool
EGLImageWrapper::FenceSync(GLContext* gl)
{
    MOZ_ASSERT(!mSync);

    if (mLibrary.IsExtensionSupported(GLLibraryEGL::KHR_fence_sync)) {
        mSync = mLibrary.fCreateSync(mDisplay,
                                     LOCAL_EGL_SYNC_FENCE,
                                     nullptr);
        
        
        
        gl->fFlush();
    }

    if (!mSync) {
        
        gl->fFinish();
    }

    return true;
}

bool
EGLImageWrapper::ClientWaitSync()
{
    if (!mSync) {
        
        return true;
    }

    
    const uint64_t ns_per_ms = 1000 * 1000;
    EGLTime timeout = 1000 * ns_per_ms;

    EGLint result = mLibrary.fClientWaitSync(mDisplay,
                                             mSync,
                                             0,
                                             timeout);
    mLibrary.fDestroySync(mDisplay, mSync);
    mSync = nullptr;

    return result == LOCAL_EGL_CONDITION_SATISFIED;
}

} 
} 
