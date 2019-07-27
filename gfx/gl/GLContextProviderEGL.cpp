




#include "mozilla/ArrayUtils.h"

#include "GLContextEGL.h"

#if defined(XP_UNIX)

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdkx.h>

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)GDK_WINDOW_XID((GdkWindow *) aWidget->GetNativeData(NS_NATIVE_WINDOW))
#elif defined(MOZ_WIDGET_QT)
#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)(aWidget->GetNativeData(NS_NATIVE_SHAREABLE_WINDOW))
#elif defined(MOZ_WIDGET_GONK)
#define GET_NATIVE_WINDOW(aWidget) ((EGLNativeWindowType)aWidget->GetNativeData(NS_NATIVE_WINDOW))
#include "HwcComposer2D.h"
#include "libdisplay/GonkDisplay.h"
#endif

#if defined(ANDROID)

#if defined(MOZ_WIDGET_ANDROID)
#include "AndroidBridge.h"
#endif

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)

# if defined(MOZ_WIDGET_GONK)
#  include "cutils/properties.h"
#  include <ui/GraphicBuffer.h>

using namespace android;
# endif

#endif

#define GLES2_LIB "libGLESv2.so"
#define GLES2_LIB2 "libGLESv2.so.2"

#elif defined(XP_WIN)

#include "nsIFile.h"

#define GLES2_LIB "libGLESv2.dll"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>


class AutoDestroyHWND {
public:
    AutoDestroyHWND(HWND aWnd = nullptr)
        : mWnd(aWnd)
    {
    }

    ~AutoDestroyHWND() {
        if (mWnd) {
            ::DestroyWindow(mWnd);
        }
    }

    operator HWND() {
        return mWnd;
    }

    HWND forget() {
        HWND w = mWnd;
        mWnd = nullptr;
        return w;
    }

    HWND operator=(HWND aWnd) {
        if (mWnd && mWnd != aWnd) {
            ::DestroyWindow(mWnd);
        }
        mWnd = aWnd;
        return mWnd;
    }

    HWND mWnd;
};

#else

#error "Platform not recognized"

#endif

#include "mozilla/Preferences.h"
#include "gfxUtils.h"
#include "gfxFailure.h"
#include "gfxASurface.h"
#include "gfxPlatform.h"
#include "GLContextProvider.h"
#include "GLLibraryEGL.h"
#include "TextureImageEGL.h"
#include "nsDebug.h"
#include "nsThreadUtils.h"

#include "nsIWidget.h"

#include "gfxCrashReporterUtils.h"

#include "ScopedGLHelpers.h"
#include "GLBlitHelper.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

#define ADD_ATTR_2(_array, _k, _v) do {         \
    (_array).AppendElement(_k);                 \
    (_array).AppendElement(_v);                 \
} while (0)

#define ADD_ATTR_1(_array, _k) do {             \
    (_array).AppendElement(_k);                 \
} while (0)

static bool
CreateConfig(EGLConfig* aConfig);




#define EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS \
     LOCAL_EGL_NONE, 0, 0, 0

static EGLint gTerminationAttribs[] = {
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static EGLint gContextAttribs[] = {
    LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static EGLint gContextAttribsRobustness[] = {
    LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
    
    LOCAL_EGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_EXT, LOCAL_EGL_LOSE_CONTEXT_ON_RESET_EXT,
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static int
next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

static bool
is_power_of_two(int v)
{
    NS_ASSERTION(v >= 0, "bad value");

    if (v == 0)
        return true;

    return (v & (v-1)) == 0;
}

static void
DestroySurface(EGLSurface oldSurface) {
    if (oldSurface != EGL_NO_SURFACE) {
        sEGLLibrary.fMakeCurrent(EGL_DISPLAY(),
                                 EGL_NO_SURFACE, EGL_NO_SURFACE,
                                 EGL_NO_CONTEXT);
        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), oldSurface);
    }
}

static EGLSurface
CreateSurfaceForWindow(nsIWidget* widget, const EGLConfig& config) {
    EGLSurface newSurface = EGL_NO_SURFACE;

    #ifdef MOZ_WIDGET_ANDROID
        mozilla::AndroidBridge::Bridge()->RegisterCompositor();
        newSurface = mozilla::AndroidBridge::Bridge()->CreateEGLSurfaceForCompositor();
        if (newSurface == EGL_NO_SURFACE) {
            return EGL_NO_SURFACE;
        }
    #else
        MOZ_ASSERT(widget != nullptr);
        newSurface = sEGLLibrary.fCreateWindowSurface(EGL_DISPLAY(), config, GET_NATIVE_WINDOW(widget), 0);
    #endif
    return newSurface;
}

GLContextEGL::GLContextEGL(
                  const SurfaceCaps& caps,
                  GLContext* shareContext,
                  bool isOffscreen,
                  EGLConfig config,
                  EGLSurface surface,
                  EGLContext context)
    : GLContext(caps, shareContext, isOffscreen)
    , mConfig(config)
    , mSurface(surface)
    , mSurfaceOverride(EGL_NO_SURFACE)
    , mContext(context)
    , mThebesSurface(nullptr)
    , mBound(false)
    , mIsPBuffer(false)
    , mIsDoubleBuffered(false)
    , mCanBindToTexture(false)
    , mShareWithEGLImage(false)
    , mOwnsContext(true)
{
    
    SetProfileVersion(ContextProfile::OpenGLES, 200);

#ifdef DEBUG
    printf_stderr("Initializing context %p surface %p on display %p\n", mContext, mSurface, EGL_DISPLAY());
#endif
}

GLContextEGL::~GLContextEGL()
{
    MarkDestroyed();

    
    if (!mOwnsContext) {
        return;
    }

#ifdef DEBUG
    printf_stderr("Destroying context %p surface %p on display %p\n", mContext, mSurface, EGL_DISPLAY());
#endif

    sEGLLibrary.fDestroyContext(EGL_DISPLAY(), mContext);
    sEGLLibrary.UnsetCachedCurrentContext();

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION < 17
    if (!mIsOffscreen) {
      
      
      
      
      
      return;
    }
#endif

    mozilla::gl::DestroySurface(mSurface);
}

bool
GLContextEGL::Init()
{
#if defined(ANDROID)
    
    
    if (!OpenLibrary(APITRACE_LIB))
#endif
        if (!OpenLibrary(GLES2_LIB)) {
#if defined(XP_UNIX)
            if (!OpenLibrary(GLES2_LIB2)) {
                NS_WARNING("Couldn't load GLES2 LIB.");
                return false;
            }
#endif
        }

    SetupLookupFunction();
    if (!InitWithPrefix("gl", true))
        return false;

    bool current = MakeCurrent();
    if (!current) {
        gfx::LogFailure(NS_LITERAL_CSTRING(
            "Couldn't get device attachments for device."));
        return false;
    }

    PR_STATIC_ASSERT(sizeof(GLint) >= sizeof(int32_t));
    mMaxTextureImageSize = INT32_MAX;

    mShareWithEGLImage = sEGLLibrary.HasKHRImageBase() &&
                          sEGLLibrary.HasKHRImageTexture2D() &&
                          IsExtensionSupported(OES_EGL_image);

    return true;
}

bool
GLContextEGL::BindTexImage()
{
    if (!mSurface)
        return false;

    if (mBound && !ReleaseTexImage())
        return false;

    EGLBoolean success = sEGLLibrary.fBindTexImage(EGL_DISPLAY(),
        (EGLSurface)mSurface, LOCAL_EGL_BACK_BUFFER);
    if (success == LOCAL_EGL_FALSE)
        return false;

    mBound = true;
    return true;
}

bool
GLContextEGL::ReleaseTexImage()
{
    if (!mBound)
        return true;

    if (!mSurface)
        return false;

    EGLBoolean success;
    success = sEGLLibrary.fReleaseTexImage(EGL_DISPLAY(),
                                            (EGLSurface)mSurface,
                                            LOCAL_EGL_BACK_BUFFER);
    if (success == LOCAL_EGL_FALSE)
        return false;

    mBound = false;
    return true;
}

void
GLContextEGL::SetEGLSurfaceOverride(EGLSurface surf) {
    if (Screen()) {
        




        Screen()->AssureBlitted();
    }

    mSurfaceOverride = surf;
    MakeCurrent(true);
}

bool
GLContextEGL::MakeCurrentImpl(bool aForce) {
    bool succeeded = true;

    
    
    
    bool hasDifferentContext = false;
    if (sEGLLibrary.CachedCurrentContext() != mContext) {
        
        
        if (sEGLLibrary.fGetCurrentContext() != mContext) {
            hasDifferentContext = true;
        } else {
            sEGLLibrary.SetCachedCurrentContext(mContext);
        }
    }

    if (aForce || hasDifferentContext) {
        EGLSurface surface = mSurfaceOverride != EGL_NO_SURFACE
                              ? mSurfaceOverride
                              : mSurface;
        if (surface == EGL_NO_SURFACE) {
            return false;
        }
        succeeded = sEGLLibrary.fMakeCurrent(EGL_DISPLAY(),
                                              surface, surface,
                                              mContext);
        if (!succeeded) {
            int eglError = sEGLLibrary.fGetError();
            if (eglError == LOCAL_EGL_CONTEXT_LOST) {
                mContextLost = true;
                NS_WARNING("EGL context has been lost.");
            } else {
                NS_WARNING("Failed to make GL context current!");
#ifdef DEBUG
                printf_stderr("EGL Error: 0x%04x\n", eglError);
#endif
            }
        } else {
            sEGLLibrary.SetCachedCurrentContext(mContext);
        }
    } else {
        MOZ_ASSERT(sEGLLibrary.CachedCurrentContextMatches());
    }

    return succeeded;
}

bool
GLContextEGL::IsCurrent() {
    return sEGLLibrary.fGetCurrentContext() == mContext;
}

bool
GLContextEGL::RenewSurface() {
    if (!mOwnsContext) {
        return false;
    }
#ifndef MOZ_WIDGET_ANDROID
    MOZ_CRASH("unimplemented");
    
    
    
    
    
#endif
    
    
    ReleaseSurface();
    mSurface = mozilla::gl::CreateSurfaceForWindow(nullptr, mConfig); 
    if (mSurface == EGL_NO_SURFACE) {
        return false;
    }
    return MakeCurrent(true);
}

void
GLContextEGL::ReleaseSurface() {
    if (mOwnsContext) {
        mozilla::gl::DestroySurface(mSurface);
    }
    if (mSurface == mSurfaceOverride) {
        mSurfaceOverride = EGL_NO_SURFACE;
    }
    mSurface = EGL_NO_SURFACE;
}

bool
GLContextEGL::SetupLookupFunction()
{
    mLookupFunc = (PlatformLookupFunction)sEGLLibrary.mSymbols.fGetProcAddress;
    return true;
}

bool
GLContextEGL::SwapBuffers()
{
    EGLSurface surface = mSurfaceOverride != EGL_NO_SURFACE
                          ? mSurfaceOverride
                          : mSurface;
    if (surface) {
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION < 17
        if (!mIsOffscreen) {
            
            return true;
        }
#endif
        return sEGLLibrary.fSwapBuffers(EGL_DISPLAY(), surface);
    } else {
        return false;
    }
}



void
GLContextEGL::HoldSurface(gfxASurface *aSurf) {
    mThebesSurface = aSurf;
}

 EGLSurface
GLContextEGL::CreateSurfaceForWindow(nsIWidget* aWidget)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        MOZ_CRASH("Failed to load EGL library!\n");
        return nullptr;
    }

    EGLConfig config;
    if (!CreateConfig(&config)) {
        MOZ_CRASH("Failed to create EGLConfig!\n");
        return nullptr;
    }

    EGLSurface surface = mozilla::gl::CreateSurfaceForWindow(aWidget, config);
    return surface;
}

 void
GLContextEGL::DestroySurface(EGLSurface aSurface)
{
    if (aSurface != EGL_NO_SURFACE) {
        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), aSurface);
    }
}

already_AddRefed<GLContextEGL>
GLContextEGL::CreateGLContext(const SurfaceCaps& caps,
                GLContextEGL *shareContext,
                bool isOffscreen,
                EGLConfig config,
                EGLSurface surface)
{
    if (sEGLLibrary.fBindAPI(LOCAL_EGL_OPENGL_ES_API) == LOCAL_EGL_FALSE) {
        NS_WARNING("Failed to bind API to GLES!");
        return nullptr;
    }

    EGLContext eglShareContext = shareContext ? shareContext->mContext
                                              : EGL_NO_CONTEXT;
    EGLint* attribs = sEGLLibrary.HasRobustness() ? gContextAttribsRobustness
                                                  : gContextAttribs;

    EGLContext context = sEGLLibrary.fCreateContext(EGL_DISPLAY(),
                                                    config,
                                                    eglShareContext,
                                                    attribs);
    if (!context && shareContext) {
        shareContext = nullptr;
        context = sEGLLibrary.fCreateContext(EGL_DISPLAY(),
                                              config,
                                              EGL_NO_CONTEXT,
                                              attribs);
    }
    if (!context) {
        NS_WARNING("Failed to create EGLContext!");
        return nullptr;
    }

    nsRefPtr<GLContextEGL> glContext = new GLContextEGL(caps,
                                                        shareContext,
                                                        isOffscreen,
                                                        config,
                                                        surface,
                                                        context);

    if (!glContext->Init())
        return nullptr;

    return glContext.forget();
}

EGLSurface
GLContextEGL::CreatePBufferSurfaceTryingPowerOfTwo(EGLConfig config,
                                                   EGLenum bindToTextureFormat,
                                                   gfxIntSize& pbsize)
{
    nsTArray<EGLint> pbattrs(16);
    EGLSurface surface = nullptr;

TRY_AGAIN_POWER_OF_TWO:
    pbattrs.Clear();
    pbattrs.AppendElement(LOCAL_EGL_WIDTH); pbattrs.AppendElement(pbsize.width);
    pbattrs.AppendElement(LOCAL_EGL_HEIGHT); pbattrs.AppendElement(pbsize.height);

    if (bindToTextureFormat != LOCAL_EGL_NONE) {
        pbattrs.AppendElement(LOCAL_EGL_TEXTURE_TARGET);
        pbattrs.AppendElement(LOCAL_EGL_TEXTURE_2D);

        pbattrs.AppendElement(LOCAL_EGL_TEXTURE_FORMAT);
        pbattrs.AppendElement(bindToTextureFormat);
    }

    for (size_t i = 0; i < MOZ_ARRAY_LENGTH(gTerminationAttribs); i++) {
      pbattrs.AppendElement(gTerminationAttribs[i]);
    }

    surface = sEGLLibrary.fCreatePbufferSurface(EGL_DISPLAY(), config, &pbattrs[0]);
    if (!surface) {
        if (!is_power_of_two(pbsize.width) ||
            !is_power_of_two(pbsize.height))
        {
            if (!is_power_of_two(pbsize.width))
                pbsize.width = next_power_of_two(pbsize.width);
            if (!is_power_of_two(pbsize.height))
                pbsize.height = next_power_of_two(pbsize.height);

            NS_WARNING("Failed to create pbuffer, trying power of two dims");
            goto TRY_AGAIN_POWER_OF_TWO;
        }

        NS_WARNING("Failed to create pbuffer surface");
        return nullptr;
    }

    return surface;
}

static const EGLint kEGLConfigAttribsOffscreenPBuffer[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_PBUFFER_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    
    LOCAL_EGL_RED_SIZE,        8,
    LOCAL_EGL_GREEN_SIZE,      8,
    LOCAL_EGL_BLUE_SIZE,       8,
    LOCAL_EGL_ALPHA_SIZE,      0,
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static const EGLint kEGLConfigAttribsRGB16[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        5,
    LOCAL_EGL_GREEN_SIZE,      6,
    LOCAL_EGL_BLUE_SIZE,       5,
    LOCAL_EGL_ALPHA_SIZE,      0,
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static const EGLint kEGLConfigAttribsRGB24[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        8,
    LOCAL_EGL_GREEN_SIZE,      8,
    LOCAL_EGL_BLUE_SIZE,       8,
    LOCAL_EGL_ALPHA_SIZE,      0,
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static const EGLint kEGLConfigAttribsRGBA32[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        8,
    LOCAL_EGL_GREEN_SIZE,      8,
    LOCAL_EGL_BLUE_SIZE,       8,
    LOCAL_EGL_ALPHA_SIZE,      8,
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
    LOCAL_EGL_FRAMEBUFFER_TARGET_ANDROID, LOCAL_EGL_TRUE,
#endif
    EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
};

static bool
CreateConfig(EGLConfig* aConfig, int32_t depth)
{
    EGLConfig configs[64];
    const EGLint* attribs;
    EGLint ncfg = ArrayLength(configs);

    switch (depth) {
        case 16:
            attribs = kEGLConfigAttribsRGB16;
            break;
        case 24:
            attribs = kEGLConfigAttribsRGB24;
            break;
        case 32:
            attribs = kEGLConfigAttribsRGBA32;
            break;
        default:
            NS_ERROR("Unknown pixel depth");
            return false;
    }

    if (!sEGLLibrary.fChooseConfig(EGL_DISPLAY(), attribs,
                                   configs, ncfg, &ncfg) ||
        ncfg < 1) {
        return false;
    }

#ifdef MOZ_WIDGET_GONK
    
    
    
    
    
    
    
    
    for (int j = 0; j < ncfg; ++j) {
        EGLConfig config = configs[j];
        EGLint format;
        if (sEGLLibrary.fGetConfigAttrib(EGL_DISPLAY(), config,
                                         LOCAL_EGL_NATIVE_VISUAL_ID, &format) &&
            format == GetGonkDisplay()->surfaceformat)
        {
            *aConfig = config;
            return true;
        }
    }
#endif

    for (int j = 0; j < ncfg; ++j) {
        EGLConfig config = configs[j];
        EGLint r, g, b, a;
        if (sEGLLibrary.fGetConfigAttrib(EGL_DISPLAY(), config,
                                         LOCAL_EGL_RED_SIZE, &r) &&
            sEGLLibrary.fGetConfigAttrib(EGL_DISPLAY(), config,
                                         LOCAL_EGL_GREEN_SIZE, &g) &&
            sEGLLibrary.fGetConfigAttrib(EGL_DISPLAY(), config,
                                         LOCAL_EGL_BLUE_SIZE, &b) &&
            sEGLLibrary.fGetConfigAttrib(EGL_DISPLAY(), config,
                                         LOCAL_EGL_ALPHA_SIZE, &a) &&
            ((depth == 16 && r == 5 && g == 6 && b == 5) ||
             (depth == 24 && r == 8 && g == 8 && b == 8) ||
             (depth == 32 && r == 8 && g == 8 && b == 8 && a == 8)))
        {
            *aConfig = config;
            return true;
        }
    }
    return false;
}






static bool
CreateConfig(EGLConfig* aConfig)
{
    int32_t depth = gfxPlatform::GetPlatform()->GetScreenDepth();
    if (!CreateConfig(aConfig, depth)) {
#ifdef MOZ_WIDGET_ANDROID
        
        
        if (depth == 16) {
            return CreateConfig(aConfig, 24);
        }
        
        
        if (depth == 24) {
            return CreateConfig(aConfig, 16);
        }
#endif
        return false;
    } else {
        return true;
    }
}

already_AddRefed<GLContext>
GLContextProviderEGL::CreateWrappingExisting(void* aContext, void* aSurface)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        MOZ_CRASH("Failed to load EGL library!\n");
        return nullptr;
    }

    if (aContext && aSurface) {
        SurfaceCaps caps = SurfaceCaps::Any();
        EGLConfig config = EGL_NO_CONFIG;
        nsRefPtr<GLContextEGL> glContext =
            new GLContextEGL(caps,
                             nullptr, false,
                             config, (EGLSurface)aSurface, (EGLContext)aContext);

        glContext->SetIsDoubleBuffered(true);
        glContext->mOwnsContext = false;

        return glContext.forget();
    }

    return nullptr;
}

already_AddRefed<GLContext>
GLContextProviderEGL::CreateForWindow(nsIWidget *aWidget)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        MOZ_CRASH("Failed to load EGL library!\n");
        return nullptr;
    }

    bool doubleBuffered = true;

    EGLConfig config;
    if (!CreateConfig(&config)) {
        MOZ_CRASH("Failed to create EGLConfig!\n");
        return nullptr;
    }

    EGLSurface surface = mozilla::gl::CreateSurfaceForWindow(aWidget, config);

    if (surface == EGL_NO_SURFACE) {
        MOZ_CRASH("Failed to create EGLSurface!\n");
        return nullptr;
    }

    SurfaceCaps caps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(caps,
                                      nullptr, false,
                                      config, surface);

    if (!glContext) {
        MOZ_CRASH("Failed to create EGLContext!\n");
        mozilla::gl::DestroySurface(surface);
        return nullptr;
    }

    glContext->MakeCurrent();
    glContext->SetIsDoubleBuffered(doubleBuffered);

    return glContext.forget();
}

#if defined(ANDROID)
EGLSurface
GLContextProviderEGL::CreateEGLSurface(void* aWindow)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        MOZ_CRASH("Failed to load EGL library!\n");
    }

    EGLConfig config;
    if (!CreateConfig(&config)) {
        MOZ_CRASH("Failed to create EGLConfig!\n");
    }

    MOZ_ASSERT(aWindow);

    EGLSurface surface = sEGLLibrary.fCreateWindowSurface(EGL_DISPLAY(), config, aWindow, 0);

    if (surface == EGL_NO_SURFACE) {
        MOZ_CRASH("Failed to create EGLSurface!\n");
    }

    return surface;
}

void
GLContextProviderEGL::DestroyEGLSurface(EGLSurface surface)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        MOZ_CRASH("Failed to load EGL library!\n");
    }

    sEGLLibrary.fDestroySurface(EGL_DISPLAY(), surface);
}
#endif 

already_AddRefed<GLContextEGL>
GLContextEGL::CreateEGLPBufferOffscreenContext(const gfxIntSize& size)
{
    EGLConfig config;
    EGLSurface surface;

    const EGLint numConfigs = 1; 
    EGLConfig configs[numConfigs];
    EGLint foundConfigs = 0;
    if (!sEGLLibrary.fChooseConfig(EGL_DISPLAY(),
                                   kEGLConfigAttribsOffscreenPBuffer,
                                   configs, numConfigs,
                                   &foundConfigs)
        || foundConfigs == 0)
    {
        NS_WARNING("No EGL Config for minimal PBuffer!");
        return nullptr;
    }

    
    config = configs[0];
    if (GLContext::ShouldSpew())
        sEGLLibrary.DumpEGLConfig(config);

    gfxIntSize pbSize(size);
    surface = GLContextEGL::CreatePBufferSurfaceTryingPowerOfTwo(config,
                                                                 LOCAL_EGL_NONE,
                                                                 pbSize);
    if (!surface) {
        NS_WARNING("Failed to create PBuffer for context!");
        return nullptr;
    }

    SurfaceCaps dummyCaps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(dummyCaps,
                                      nullptr, true,
                                      config, surface);
    if (!glContext) {
        NS_WARNING("Failed to create GLContext from PBuffer");
        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), surface);
        return nullptr;
    }

    if (!glContext->Init()) {
        NS_WARNING("Failed to initialize GLContext!");
        
        return nullptr;
    }

    return glContext.forget();
}

already_AddRefed<GLContextEGL>
GLContextEGL::CreateEGLPixmapOffscreenContext(const gfxIntSize& size)
{
    gfxASurface *thebesSurface = nullptr;
    EGLNativePixmapType pixmap = 0;

    if (!pixmap) {
        return nullptr;
    }

    EGLSurface surface = 0;
    EGLConfig config = 0;

    if (!config) {
        return nullptr;
    }
    MOZ_ASSERT(surface);

    SurfaceCaps dummyCaps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(dummyCaps,
                                      nullptr, true,
                                      config, surface);
    if (!glContext) {
        NS_WARNING("Failed to create GLContext from XSurface");
        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), surface);
        return nullptr;
    }

    if (!glContext->Init()) {
        NS_WARNING("Failed to initialize GLContext!");
        
        return nullptr;
    }

    glContext->HoldSurface(thebesSurface);

    return glContext.forget();
}

already_AddRefed<GLContext>
GLContextProviderEGL::CreateHeadless(bool)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nullptr;
    }

    gfxIntSize dummySize = gfxIntSize(16, 16);
    nsRefPtr<GLContext> glContext;
    glContext = GLContextEGL::CreateEGLPBufferOffscreenContext(dummySize);
    if (!glContext)
        return nullptr;

    return glContext.forget();
}



already_AddRefed<GLContext>
GLContextProviderEGL::CreateOffscreen(const gfxIntSize& size,
                                      const SurfaceCaps& caps,
                                      bool requireCompatProfile)
{
    nsRefPtr<GLContext> glContext = CreateHeadless(requireCompatProfile);
    if (!glContext)
        return nullptr;

    if (!glContext->InitOffscreen(size, caps))
        return nullptr;

    return glContext.forget();
}




GLContext*
GLContextProviderEGL::GetGlobalContext()
{
    return nullptr;
}

void
GLContextProviderEGL::Shutdown()
{
}

} 
} 

#undef EGL_ATTRIBS_LIST_SAFE_TERMINATION_WORKING_AROUND_BUGS
