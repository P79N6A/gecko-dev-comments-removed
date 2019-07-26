




#include "SharedSurfaceIO.h"
#include "GLContext.h"
#include "gfxImageSurface.h"
#include "mozilla/gfx/MacIOSurface.h"
#include "mozilla/DebugOnly.h"

namespace mozilla {
namespace gl {

using namespace gfx;

 SharedSurface_IOSurface*
SharedSurface_IOSurface::Create(MacIOSurface* surface, GLContext *gl, bool hasAlpha)
{
    gfxIntSize size(surface->GetWidth(), surface->GetHeight());
    return new SharedSurface_IOSurface(surface, gl, size, hasAlpha);
}

void
SharedSurface_IOSurface::Fence()
{
    mGL->MakeCurrent();
    mGL->fFlush();
}

bool
SharedSurface_IOSurface::ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                    GLenum format, GLenum type, GLvoid *pixels)
{
    
    
    
    
    
    ScopedTexture texture(mGL);
    ScopedBindTexture bindTex(mGL, texture.Texture());
    mGL->fCopyTexImage2D(LOCAL_GL_TEXTURE_2D, 0,
                         HasAlpha() ? LOCAL_GL_RGBA : LOCAL_GL_RGB,
                         x, y,
                         width, height, 0);

    ScopedFramebufferForTexture fb(mGL, texture.Texture());
    ScopedBindFramebuffer bindFB(mGL, fb.FB());

    mGL->fReadPixels(0, 0, width, height, format, type, pixels);
    return true;
}

SharedSurface_IOSurface::SharedSurface_IOSurface(MacIOSurface* surface,
                                                 GLContext* gl,
                                                 const gfxIntSize& size,
                                                 bool hasAlpha)
  : SharedSurface_GL(SharedSurfaceType::IOSurface, AttachmentType::GLTexture, gl, size, hasAlpha)
  , mSurface(surface)
{
    mGL->MakeCurrent();
    mGL->fGenTextures(1, &mTexture);

    ScopedBindTexture texture(mGL, mTexture, LOCAL_GL_TEXTURE_RECTANGLE_ARB);

    mGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_MIN_FILTER,
                        LOCAL_GL_LINEAR);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_MAG_FILTER,
                        LOCAL_GL_LINEAR);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_WRAP_S,
                        LOCAL_GL_CLAMP_TO_EDGE);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_WRAP_T,
                        LOCAL_GL_CLAMP_TO_EDGE);

    void *nativeCtx = mGL->GetNativeData(GLContext::NativeGLContext);
    MOZ_ASSERT(nativeCtx);

    surface->CGLTexImageIOSurface2D(nativeCtx);
}

SharedSurface_IOSurface::~SharedSurface_IOSurface()
{
    if (mTexture) {
        DebugOnly<bool> success = mGL->MakeCurrent();
        MOZ_ASSERT(success);
        mGL->fDeleteTextures(1, &mTexture);
    }
}

SharedSurface*
SurfaceFactory_IOSurface::CreateShared(const gfxIntSize& size)
{
    bool hasAlpha = mReadCaps.alpha;
    RefPtr<MacIOSurface> surf =
        MacIOSurface::CreateIOSurface(size.width, size.height, 1.0, hasAlpha);

    return SharedSurface_IOSurface::Create(surf, mGL, hasAlpha);
}

}
}
