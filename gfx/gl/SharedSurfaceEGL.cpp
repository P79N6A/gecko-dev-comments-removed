




#include "SharedSurfaceEGL.h"

#include "GLContext.h"
#include "GLBlitHelper.h"
#include "ScopedGLHelpers.h"
#include "SharedSurfaceGL.h"
#include "SurfaceFactory.h"
#include "GLLibraryEGL.h"
#include "TextureGarbageBin.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

SharedSurface_EGLImage*
SharedSurface_EGLImage::Create(GLContext* prodGL,
                               const GLFormats& formats,
                               const gfxIntSize& size,
                               bool hasAlpha,
                               EGLContext context)
{
    GLLibraryEGL* egl = prodGL->GetLibraryEGL();
    MOZ_ASSERT(egl);

    if (!HasExtensions(egl, prodGL))
        return nullptr;

    MOZ_ALWAYS_TRUE(prodGL->MakeCurrent());
    GLuint prodTex = CreateTextureForOffscreen(prodGL, formats, size);
    if (!prodTex)
        return nullptr;

    return new SharedSurface_EGLImage(prodGL, egl,
                                      size, hasAlpha,
                                      formats, prodTex);
}


bool
SharedSurface_EGLImage::HasExtensions(GLLibraryEGL* egl, GLContext* gl)
{
    return egl->HasKHRImageBase() &&
           egl->IsExtensionSupported(GLLibraryEGL::KHR_gl_texture_2D_image) &&
           gl->IsExtensionSupported(GLContext::OES_EGL_image);
}

SharedSurface_EGLImage::SharedSurface_EGLImage(GLContext* gl,
                                               GLLibraryEGL* egl,
                                               const gfxIntSize& size,
                                               bool hasAlpha,
                                               const GLFormats& formats,
                                               GLuint prodTex)
    : SharedSurface_GL(SharedSurfaceType::EGLImageShare,
                        AttachmentType::GLTexture,
                        gl,
                        size,
                        hasAlpha)
    , mMutex("SharedSurface_EGLImage mutex")
    , mEGL(egl)
    , mFormats(formats)
    , mProdTex(prodTex)
    , mProdTexForPipe(0)
    , mImage(0)
    , mCurConsGL(nullptr)
    , mConsTex(0)
    , mSync(0)
    , mPipeFailed(false)
    , mPipeComplete(false)
    , mPipeActive(false)
{}

SharedSurface_EGLImage::~SharedSurface_EGLImage()
{
    mEGL->fDestroyImage(Display(), mImage);
    mImage = 0;

    mGL->MakeCurrent();
    mGL->fDeleteTextures(1, &mProdTex);
    mProdTex = 0;

    if (mProdTexForPipe) {
        mGL->fDeleteTextures(1, &mProdTexForPipe);
        mProdTexForPipe = 0;
    }

    if (mConsTex) {
        MOZ_ASSERT(mGarbageBin);
        mGarbageBin->Trash(mConsTex);
        mConsTex = 0;
    }

    if (mSync) {
        
        
        mEGL->fDestroySync(Display(), mSync);
        mSync = 0;
    }
}

void
SharedSurface_EGLImage::LockProdImpl()
{
    MutexAutoLock lock(mMutex);

    if (!mPipeComplete)
        return;

    if (mPipeActive)
        return;

    mGL->BlitHelper()->BlitTextureToTexture(mProdTex, mProdTexForPipe, Size(), Size());
    mGL->fDeleteTextures(1, &mProdTex);
    mProdTex = mProdTexForPipe;
    mProdTexForPipe = 0;
    mPipeActive = true;
}

static bool
CreateTexturePipe(GLLibraryEGL* const egl, GLContext* const gl,
                  const GLFormats& formats, const gfxIntSize& size,
                  GLuint* const out_tex, EGLImage* const out_image)
{
    MOZ_ASSERT(out_tex && out_image);
    *out_tex = 0;
    *out_image = 0;

    GLuint tex = CreateTextureForOffscreen(gl, formats, size);
    if (!tex)
        return false;

    EGLContext context = gl->GetEGLContext();
    MOZ_ASSERT(context);
    EGLClientBuffer buffer = reinterpret_cast<EGLClientBuffer>(tex);
    EGLImage image = egl->fCreateImage(egl->Display(), context,
                                       LOCAL_EGL_GL_TEXTURE_2D, buffer,
                                       nullptr);
    if (!image) {
        gl->fDeleteTextures(1, &tex);
        return false;
    }

    
    *out_tex = tex;
    *out_image = image;
    return true;
}

void
SharedSurface_EGLImage::Fence()
{
    MutexAutoLock lock(mMutex);
    mGL->MakeCurrent();

    if (!mPipeActive) {
        MOZ_ASSERT(!mSync);
        MOZ_ASSERT(!mPipeComplete);

        if (!mPipeFailed) {
            if (!CreateTexturePipe(mEGL, mGL, mFormats, Size(),
                                   &mProdTexForPipe, &mImage))
            {
                mPipeFailed = true;
            }
        }

        if (!mPixels) {
            gfxImageFormat format =
                  HasAlpha() ? gfxImageFormatARGB32
                             : gfxImageFormatRGB24;
            mPixels = new gfxImageSurface(Size(), format);
        }

        mPixels->Flush();
        mGL->ReadScreenIntoImageSurface(mPixels);
        mPixels->MarkDirty();
        return;
    }
    MOZ_ASSERT(mPipeActive);
    MOZ_ASSERT(mCurConsGL);

    if (mEGL->IsExtensionSupported(GLLibraryEGL::KHR_fence_sync) &&
        mGL->IsExtensionSupported(GLContext::OES_EGL_sync))
    {
        if (mSync) {
            MOZ_ALWAYS_TRUE( mEGL->fDestroySync(Display(), mSync) );
            mSync = 0;
        }

        mSync = mEGL->fCreateSync(Display(),
                                  LOCAL_EGL_SYNC_FENCE,
                                  nullptr);
        if (mSync) {
            mGL->fFlush();
            return;
        }
    }

    MOZ_ASSERT(!mSync);
    mGL->fFinish();
}

bool
SharedSurface_EGLImage::WaitSync()
{
    MutexAutoLock lock(mMutex);
    if (!mSync) {
        
        return true;
    }
    MOZ_ASSERT(mEGL->IsExtensionSupported(GLLibraryEGL::KHR_fence_sync));

    
    
    
    
    
    EGLint status = mEGL->fClientWaitSync(Display(),
                                          mSync,
                                          0,
                                          LOCAL_EGL_FOREVER);

    if (status != LOCAL_EGL_CONDITION_SATISFIED) {
        return false;
    }

    MOZ_ALWAYS_TRUE( mEGL->fDestroySync(Display(), mSync) );
    mSync = 0;

    return true;
}


EGLDisplay
SharedSurface_EGLImage::Display() const
{
    return mEGL->Display();
}

GLuint
SharedSurface_EGLImage::AcquireConsumerTexture(GLContext* consGL)
{
    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(!mCurConsGL || consGL == mCurConsGL);
    if (mPipeFailed)
        return 0;

    if (mPipeActive) {
        MOZ_ASSERT(mConsTex);

        return mConsTex;
    }

    if (!mConsTex) {
        consGL->fGenTextures(1, &mConsTex);
        ScopedBindTexture autoTex(consGL, mConsTex);
        consGL->fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, mImage);

        mPipeComplete = true;
        mCurConsGL = consGL;
        mGarbageBin = consGL->TexGarbageBin();
    }

    MOZ_ASSERT(consGL == mCurConsGL);
    return 0;
}

gfxImageSurface*
SharedSurface_EGLImage::GetPixels() const
{
    MutexAutoLock lock(mMutex);
    return mPixels;
}



SurfaceFactory_EGLImage*
SurfaceFactory_EGLImage::Create(GLContext* prodGL,
                                        const SurfaceCaps& caps)
{
    EGLContext context = prodGL->GetEGLContext();

    return new SurfaceFactory_EGLImage(prodGL, context, caps);
}

} 
} 
