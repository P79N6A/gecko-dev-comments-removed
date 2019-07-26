




#include "SharedSurfaceANGLE.h"

#include "GLContext.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

SurfaceFactory_ANGLEShareHandle*
SurfaceFactory_ANGLEShareHandle::Create(GLContext* gl,
                                        ID3D10Device1* d3d,
                                        const SurfaceCaps& caps)
{
    GLLibraryEGL* egl = &sEGLLibrary;
    if (!egl)
        return nullptr;

    if (!egl->IsExtensionSupported(
            GLLibraryEGL::ANGLE_surface_d3d_texture_2d_share_handle))
    {
        return nullptr;
    }

    return new SurfaceFactory_ANGLEShareHandle(gl, egl, d3d, caps);
}

EGLDisplay
SharedSurface_ANGLEShareHandle::Display()
{
    return mEGL->Display();
}


SharedSurface_ANGLEShareHandle::~SharedSurface_ANGLEShareHandle()
{
    mEGL->fDestroySurface(Display(), mPBuffer);
}

void
SharedSurface_ANGLEShareHandle::LockProdImpl()
{
    mGL->SetEGLSurfaceOverride(mPBuffer);
}

void
SharedSurface_ANGLEShareHandle::UnlockProdImpl()
{
}


void
SharedSurface_ANGLEShareHandle::Fence()
{
    mGL->fFinish();
}

bool
SharedSurface_ANGLEShareHandle::WaitSync()
{
    
    return true;
}

static void
FillPBufferAttribs_ByBits(nsTArray<EGLint>& aAttrs,
                          int redBits, int greenBits,
                          int blueBits, int alphaBits,
                          int depthBits, int stencilBits)
{
    aAttrs.Clear();

#if defined(A1) || defined(A2)
#error The temp-macro names we want are already defined.
#endif

#define A1(_x)      do { aAttrs.AppendElement(_x); } while (0)
#define A2(_x,_y)   do { A1(_x); A1(_y); } while (0)

    A2(LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT);
    A2(LOCAL_EGL_SURFACE_TYPE, LOCAL_EGL_PBUFFER_BIT);

    A2(LOCAL_EGL_RED_SIZE, redBits);
    A2(LOCAL_EGL_GREEN_SIZE, greenBits);
    A2(LOCAL_EGL_BLUE_SIZE, blueBits);
    A2(LOCAL_EGL_ALPHA_SIZE, alphaBits);

    A2(LOCAL_EGL_DEPTH_SIZE, depthBits);
    A2(LOCAL_EGL_STENCIL_SIZE, stencilBits);

    A1(LOCAL_EGL_NONE);
#undef A1
#undef A2
}

static void
FillPBufferAttribs_BySizes(nsTArray<EGLint>& attribs,
                           bool bpp16, bool hasAlpha,
                           int depthBits, int stencilBits)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 0;

    if (bpp16) {
        if (hasAlpha) {
            red = green = blue = alpha = 4;
        } else {
            red = 5;
            green = 6;
            blue = 5;
        }
    } else {
        red = green = blue = 8;
        if (hasAlpha)
            alpha = 8;
    }

    FillPBufferAttribs_ByBits(attribs,
                              red, green, blue, alpha,
                              depthBits, stencilBits);
}

static EGLConfig
ChooseConfig(GLContext* gl,
             GLLibraryEGL* egl,
             const SurfaceCaps& caps)
{
    MOZ_ASSERT(egl);
    MOZ_ASSERT(caps.color);

    
    int depthBits = caps.depth ? 16 : 0;
    int stencilBits = caps.stencil ? 8 : 0;

    
    nsTArray<EGLint> attribs(32);
    FillPBufferAttribs_BySizes(attribs,
                               caps.bpp16, caps.alpha,
                               depthBits, stencilBits);

    
    EGLConfig configs[64];
    int numConfigs = sizeof(configs)/sizeof(EGLConfig);
    int foundConfigs = 0;

    if (!egl->fChooseConfig(egl->Display(),
                            attribs.Elements(),
                            configs, numConfigs,
                            &foundConfigs) ||
        !foundConfigs)
    {
        NS_WARNING("No configs found for the requested formats.");
        return EGL_NO_CONFIG;
    }

    
    
    EGLConfig config = configs[0];

    if (gl->DebugMode()) {
        egl->DumpEGLConfig(config);
    }

    return config;
}



static EGLSurface CreatePBufferSurface(GLLibraryEGL* egl,
                                       EGLDisplay display,
                                       EGLConfig config,
                                       const gfx::IntSize& size)
{
    EGLint attribs[] = {
        LOCAL_EGL_WIDTH, size.width,
        LOCAL_EGL_HEIGHT, size.height,
        LOCAL_EGL_NONE
    };

    EGLSurface surface = egl->fCreatePbufferSurface(display, config, attribs);

    return surface;
}

SharedSurface_ANGLEShareHandle*
SharedSurface_ANGLEShareHandle::Create(GLContext* gl, ID3D10Device1* d3d,
                                       EGLContext context, EGLConfig config,
                                       const gfx::IntSize& size, bool hasAlpha)
{
    GLLibraryEGL* egl = &sEGLLibrary;
    MOZ_ASSERT(egl);
    MOZ_ASSERT(egl->IsExtensionSupported(
               GLLibraryEGL::ANGLE_surface_d3d_texture_2d_share_handle));

    if (!context || !config)
        return nullptr;

    EGLDisplay display = egl->Display();
    EGLSurface pbuffer = CreatePBufferSurface(egl, display, config, size);
    if (!pbuffer)
        return nullptr;


    
    HANDLE shareHandle = nullptr;
    nsRefPtr<ID3D10Texture2D> texture;
    nsRefPtr<ID3D10ShaderResourceView> srv;

    
    
    bool failed = true;
    HRESULT hr;

    
    if (!egl->fQuerySurfacePointerANGLE(
            display,
            pbuffer,
            LOCAL_EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE,
            &shareHandle))
    {
        NS_ERROR("Failed to grab ShareHandle for PBuffer!");
        goto CleanUpIfFailed;
    }

    
    
    hr = d3d->OpenSharedResource(shareHandle,
                                 __uuidof(ID3D10Texture2D),
                                 getter_AddRefs(texture));
    if (FAILED(hr)) {
        NS_ERROR("Failed to open shared resource!");
        goto CleanUpIfFailed;
    }

    hr = d3d->CreateShaderResourceView(texture, nullptr, getter_AddRefs(srv));
    if (FAILED(hr)) {
        NS_ERROR("Failed to create SRV!");
        goto CleanUpIfFailed;
    }

    failed = false;

CleanUpIfFailed:
    if (failed) {
        NS_WARNING("CleanUpIfFailed");
        egl->fDestroySurface(egl->Display(), pbuffer);
        return nullptr;
    }

    return new SharedSurface_ANGLEShareHandle(gl, egl,
                                              size, hasAlpha,
                                              context, pbuffer,
                                              texture, srv);
}


SurfaceFactory_ANGLEShareHandle::SurfaceFactory_ANGLEShareHandle(GLContext* gl,
                                                                 GLLibraryEGL* egl,
                                                                 ID3D10Device1* d3d,
                                                                 const SurfaceCaps& caps)
    : SurfaceFactory_GL(gl, SharedSurfaceType::EGLSurfaceANGLE, caps)
    , mProdGL(gl)
    , mEGL(egl)
    , mConsD3D(d3d)
{
    mConfig = ChooseConfig(mProdGL, mEGL, mReadCaps);
    mContext = mProdGL->GetNativeData(GLContext::NativeGLContext);
    MOZ_ASSERT(mConfig && mContext);
}

} 
} 
