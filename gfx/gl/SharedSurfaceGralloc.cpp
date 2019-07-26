




#include "mozilla/Preferences.h"

#include "SharedSurfaceGralloc.h"

#include "GLContext.h"
#include "SharedSurfaceGL.h"
#include "SurfaceFactory.h"
#include "GLLibraryEGL.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/ShadowLayers.h"

#include "ui/GraphicBuffer.h"
#include "../layers/ipc/ShadowLayers.h"
#include "ScopedGLHelpers.h"

#include "gfxPlatform.h"
#include "gfx2DGlue.h"
#include "gfxPrefs.h"

#define DEBUG_GRALLOC
#ifdef DEBUG_GRALLOC
#define DEBUG_PRINT(...) do { printf_stderr(__VA_ARGS__); } while (0)
#else
#define DEBUG_PRINT(...) do { } while (0)
#endif

using namespace mozilla;
using namespace mozilla::gfx;
using namespace gl;
using namespace layers;
using namespace android;

SurfaceFactory_Gralloc::SurfaceFactory_Gralloc(GLContext* prodGL,
                                               const SurfaceCaps& caps,
                                               layers::ISurfaceAllocator* allocator)
    : SurfaceFactory_GL(prodGL, SharedSurfaceType::Gralloc, caps)
{
    if (caps.surfaceAllocator) {
        allocator = caps.surfaceAllocator;
    }

    MOZ_ASSERT(allocator);

    mAllocator = allocator;
}

SharedSurface_Gralloc*
SharedSurface_Gralloc::Create(GLContext* prodGL,
                              const GLFormats& formats,
                              const gfx::IntSize& size,
                              bool hasAlpha,
                              ISurfaceAllocator* allocator)
{
    GLLibraryEGL* egl = &sEGLLibrary;
    MOZ_ASSERT(egl);

    DEBUG_PRINT("SharedSurface_Gralloc::Create -------\n");

    if (!HasExtensions(egl, prodGL))
        return nullptr;

    gfxContentType type = hasAlpha ? gfxContentType::COLOR_ALPHA
                                   : gfxContentType::COLOR;

    gfxImageFormat format
      = gfxPlatform::GetPlatform()->OptimalFormatForContent(type);

    RefPtr<GrallocTextureClientOGL> grallocTC =
      new GrallocTextureClientOGL(
          allocator,
          gfx::ImageFormatToSurfaceFormat(format),
          gfx::BackendType::NONE, 
          layers::TextureFlags::DEFAULT);

    if (!grallocTC->AllocateForGLRendering(size)) {
      return nullptr;
    }

    sp<GraphicBuffer> buffer = grallocTC->GetGraphicBuffer();

    EGLDisplay display = egl->Display();
    EGLClientBuffer clientBuffer = buffer->getNativeBuffer();
    EGLint attrs[] = {
        LOCAL_EGL_NONE, LOCAL_EGL_NONE
    };
    EGLImage image = egl->fCreateImage(display,
                                       EGL_NO_CONTEXT,
                                       LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                                       clientBuffer, attrs);
    if (!image) {
        return nullptr;
    }

    prodGL->MakeCurrent();
    GLuint prodTex = 0;
    prodGL->fGenTextures(1, &prodTex);
    ScopedBindTexture autoTex(prodGL, prodTex);

    prodGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
    prodGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
    prodGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
    prodGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

    prodGL->fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, image);

    egl->fDestroyImage(display, image);

    SharedSurface_Gralloc *surf = new SharedSurface_Gralloc(prodGL, size, hasAlpha, egl, allocator, grallocTC, prodTex);

    DEBUG_PRINT("SharedSurface_Gralloc::Create: success -- surface %p, GraphicBuffer %p.\n", surf, buffer.get());

    return surf;
}


bool
SharedSurface_Gralloc::HasExtensions(GLLibraryEGL* egl, GLContext* gl)
{
    return egl->HasKHRImageBase() &&
           gl->IsExtensionSupported(GLContext::OES_EGL_image);
}

SharedSurface_Gralloc::~SharedSurface_Gralloc()
{

    DEBUG_PRINT("[SharedSurface_Gralloc %p] destroyed\n", this);

    mGL->MakeCurrent();
    mGL->fDeleteTextures(1, &mProdTex);
}

void
SharedSurface_Gralloc::Fence()
{
    
    
    
    if (gfxPrefs::GrallocFenceWithReadPixels()) {
        mGL->MakeCurrent();
        ScopedDeleteArray<char> buf(new char[4]);
        mGL->fReadPixels(0, 0, 1, 1, LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE, buf);
    }
}

bool
SharedSurface_Gralloc::WaitSync()
{
    return true;
}

void
SharedSurface_Gralloc::WaitForBufferOwnership()
{
    mTextureClient->WaitReleaseFence();
}
