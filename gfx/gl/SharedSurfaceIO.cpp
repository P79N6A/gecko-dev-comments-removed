




#include "SharedSurfaceIO.h"

#include "GLContextCGL.h"
#include "mozilla/gfx/MacIOSurface.h"
#include "mozilla/DebugOnly.h"
#include "ScopedGLHelpers.h"

namespace mozilla {
namespace gl {

 UniquePtr<SharedSurface_IOSurface>
SharedSurface_IOSurface::Create(const RefPtr<MacIOSurface>& ioSurf,
                                GLContext* gl,
                                bool hasAlpha)
{
    MOZ_ASSERT(ioSurf);
    MOZ_ASSERT(gl);

    gfx::IntSize size(ioSurf->GetWidth(), ioSurf->GetHeight());

    typedef SharedSurface_IOSurface ptrT;
    UniquePtr<ptrT> ret( new ptrT(ioSurf, gl, size, hasAlpha) );
    return Move(ret);
}

void
SharedSurface_IOSurface::Fence()
{
    mGL->MakeCurrent();
    mGL->fFlush();
}

bool
SharedSurface_IOSurface::CopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                        GLint x, GLint y, GLsizei width, GLsizei height,
                                        GLint border)
{
    






    
    
    
    
    if (width == 0 || height == 0)
        return false;

    switch (internalformat) {
    case LOCAL_GL_ALPHA:
    case LOCAL_GL_LUMINANCE:
    case LOCAL_GL_LUMINANCE_ALPHA:
        break;

    default:
        return false;
    }

    MOZ_ASSERT(mGL->IsCurrent());

    ScopedTexture destTex(mGL);
    {
        ScopedBindTexture bindTex(mGL, destTex.Texture());
        mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER,
                            LOCAL_GL_NEAREST);
        mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER,
                            LOCAL_GL_NEAREST);
        mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S,
                            LOCAL_GL_CLAMP_TO_EDGE);
        mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T,
                            LOCAL_GL_CLAMP_TO_EDGE);
        mGL->raw_fCopyTexImage2D(LOCAL_GL_TEXTURE_2D, 0, LOCAL_GL_RGBA, x, y, width,
                                 height, 0);
    }

    ScopedFramebufferForTexture tmpFB(mGL, destTex.Texture(), LOCAL_GL_TEXTURE_2D);
    ScopedBindFramebuffer bindFB(mGL, tmpFB.FB());
    mGL->raw_fCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

    return true;
}

bool
SharedSurface_IOSurface::ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                    GLenum format, GLenum type, GLvoid* pixels)
{
    
    
    
    
    
    MOZ_ASSERT(mGL->IsCurrent());

    ScopedTexture destTex(mGL);
    {
        ScopedFramebufferForTexture srcFB(mGL, ProdTexture(), ProdTextureTarget());

        ScopedBindFramebuffer bindFB(mGL, srcFB.FB());
        ScopedBindTexture bindTex(mGL, destTex.Texture());
        mGL->raw_fCopyTexImage2D(LOCAL_GL_TEXTURE_2D, 0,
                                 mHasAlpha ? LOCAL_GL_RGBA : LOCAL_GL_RGB,
                                 x, y,
                                 width, height, 0);
    }

    ScopedFramebufferForTexture destFB(mGL, destTex.Texture());

    ScopedBindFramebuffer bindFB(mGL, destFB.FB());
    mGL->raw_fReadPixels(0, 0, width, height, format, type, pixels);
    return true;
}

static void
BackTextureWithIOSurf(GLContext* gl, GLuint tex, MacIOSurface* ioSurf)
{
    MOZ_ASSERT(gl->IsCurrent());

    ScopedBindTexture texture(gl, tex, LOCAL_GL_TEXTURE_RECTANGLE_ARB);

    gl->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_MIN_FILTER,
                        LOCAL_GL_LINEAR);
    gl->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_MAG_FILTER,
                        LOCAL_GL_LINEAR);
    gl->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_WRAP_S,
                        LOCAL_GL_CLAMP_TO_EDGE);
    gl->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                        LOCAL_GL_TEXTURE_WRAP_T,
                        LOCAL_GL_CLAMP_TO_EDGE);

    CGLContextObj cgl = GLContextCGL::Cast(gl)->GetCGLContext();
    MOZ_ASSERT(cgl);

    ioSurf->CGLTexImageIOSurface2D(cgl);
}

SharedSurface_IOSurface::SharedSurface_IOSurface(const RefPtr<MacIOSurface>& ioSurf,
                                                 GLContext* gl,
                                                 const gfx::IntSize& size,
                                                 bool hasAlpha)
  : SharedSurface(SharedSurfaceType::IOSurface,
                  AttachmentType::GLTexture,
                  gl,
                  size,
                  hasAlpha)
  , mIOSurf(ioSurf)
{
    gl->MakeCurrent();
    mProdTex = 0;
    gl->fGenTextures(1, &mProdTex);
    BackTextureWithIOSurf(gl, mProdTex, mIOSurf);
}

SharedSurface_IOSurface::~SharedSurface_IOSurface()
{
    if (mProdTex) {
        DebugOnly<bool> success = mGL->MakeCurrent();
        MOZ_ASSERT(success);
        mGL->fDeleteTextures(1, &mProdTex);
    }
}




 UniquePtr<SurfaceFactory_IOSurface>
SurfaceFactory_IOSurface::Create(GLContext* gl,
                                 const SurfaceCaps& caps)
{
    gfx::IntSize maxDims(MacIOSurface::GetMaxWidth(),
                         MacIOSurface::GetMaxHeight());

    typedef SurfaceFactory_IOSurface ptrT;
    UniquePtr<ptrT> ret( new ptrT(gl, caps, maxDims) );
    return Move(ret);
}

UniquePtr<SharedSurface>
SurfaceFactory_IOSurface::CreateShared(const gfx::IntSize& size)
{
    if (size.width > mMaxDims.width ||
        size.height > mMaxDims.height)
    {
        return nullptr;
    }

    bool hasAlpha = mReadCaps.alpha;
    RefPtr<MacIOSurface> ioSurf;
    ioSurf = MacIOSurface::CreateIOSurface(size.width, size.height, 1.0,
                                           hasAlpha);

    if (!ioSurf) {
        NS_WARNING("Failed to create MacIOSurface.");
        return nullptr;
    }

    return SharedSurface_IOSurface::Create(ioSurf, mGL, hasAlpha);
}

}
}
