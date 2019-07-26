




#include "mozilla/Util.h"


#if defined(XP_UNIX)

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdkx.h>

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)GDK_WINDOW_XID((GdkWindow *) aWidget->GetNativeData(NS_NATIVE_WINDOW))
#elif defined(MOZ_WIDGET_QT)
#include <QtOpenGL/QGLContext>
#define GLdouble_defined 1

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)static_cast<QWidget*>(aWidget->GetNativeData(NS_NATIVE_SHELLWIDGET))->winId()
#elif defined(MOZ_WIDGET_GONK)
#define GET_NATIVE_WINDOW(aWidget) ((EGLNativeWindowType)aWidget->GetNativeData(NS_NATIVE_WINDOW))
#include "HwcComposer2D.h"
#endif

#if defined(MOZ_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "mozilla/X11Util.h"
#include "gfxXlibSurface.h"
#endif

#if defined(ANDROID)

#if defined(MOZ_WIDGET_ANDROID)
#include "AndroidBridge.h"
#include "nsSurfaceTexture.h"
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
    AutoDestroyHWND(HWND aWnd = NULL)
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
        mWnd = NULL;
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
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "GLContextProvider.h"
#include "GLLibraryEGL.h"
#include "nsDebug.h"
#include "nsThreadUtils.h"

#include "nsIWidget.h"

#include "gfxCrashReporterUtils.h"


#if defined(MOZ_PLATFORM_MAEMO) || defined(MOZ_WIDGET_GONK)
static bool gUseBackingSurface = true;
#else
static bool gUseBackingSurface = false;
#endif

#ifdef MOZ_WIDGET_GONK
extern nsIntRect gScreenBounds;
#endif

#define EGL_DISPLAY()        sEGLLibrary.Display()

namespace mozilla {
namespace gl {

static GLLibraryEGL sEGLLibrary;

#define ADD_ATTR_2(_array, _k, _v) do {         \
    (_array).AppendElement(_k);                 \
    (_array).AppendElement(_v);                 \
} while (0)

#define ADD_ATTR_1(_array, _k) do {             \
    (_array).AppendElement(_k);                 \
} while (0)

#ifndef MOZ_ANDROID_OMTC
static EGLSurface
CreateSurfaceForWindow(nsIWidget *aWidget, EGLConfig config);
#endif

static bool
CreateConfig(EGLConfig* aConfig);
#ifdef MOZ_X11

static EGLConfig
CreateEGLSurfaceForXSurface(gfxASurface* aSurface, EGLConfig* aConfig = nullptr);
#endif

static EGLint gContextAttribs[] = {
    LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
    LOCAL_EGL_NONE
};

static EGLint gContextAttribsRobustness[] = {
    LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
    
    LOCAL_EGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_EXT, LOCAL_EGL_LOSE_CONTEXT_ON_RESET_EXT,
    LOCAL_EGL_NONE
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

class GLContextEGL : public GLContext
{
    friend class TextureImageEGL;

    static already_AddRefed<GLContextEGL>
    CreateGLContext(const SurfaceCaps& caps,
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

public:
    GLContextEGL(const SurfaceCaps& caps,
                 GLContext* shareContext,
                 bool isOffscreen,
                 EGLConfig config,
                 EGLSurface surface,
                 EGLContext context)
        : GLContext(caps, shareContext, isOffscreen)
        , mConfig(config)
        , mSurface(surface)
        , mCurSurface(surface)
        , mContext(context)
        , mPlatformContext(nullptr)
        , mThebesSurface(nullptr)
        , mBound(false)
        , mIsPBuffer(false)
        , mIsDoubleBuffered(false)
        , mCanBindToTexture(false)
        , mShareWithEGLImage(false)
        , mTemporaryEGLImageTexture(0)
    {
        
        SetIsGLES2(true);

#ifdef DEBUG
        printf_stderr("Initializing context %p surface %p on display %p\n", mContext, mSurface, EGL_DISPLAY());
#endif
#ifdef MOZ_WIDGET_GONK
        if (!mIsOffscreen) {
            mHwc = HwcComposer2D::GetInstance();
            MOZ_ASSERT(!mHwc->Initialized());

            if (mHwc->Init(EGL_DISPLAY(), mSurface)) {
                NS_WARNING("HWComposer initialization failed!");
                mHwc = nullptr;
            }
        }
#endif
    }

    ~GLContextEGL()
    {
        if (MakeCurrent()) {
            if (mTemporaryEGLImageTexture != 0) {
                fDeleteTextures(1, &mTemporaryEGLImageTexture);
                mTemporaryEGLImageTexture = 0;
            }
        }

        MarkDestroyed();

        
        
        
        if (mPlatformContext)
            return;

#ifdef DEBUG
        printf_stderr("Destroying context %p surface %p on display %p\n", mContext, mSurface, EGL_DISPLAY());
#endif

        sEGLLibrary.fDestroyContext(EGL_DISPLAY(), mContext);
        if (mSurface && !mPlatformContext) {
            sEGLLibrary.fDestroySurface(EGL_DISPLAY(), mSurface);
        }
    }

    GLContextType GetContextType() {
        return ContextTypeEGL;
    }

    bool Init()
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

#ifdef MOZ_WIDGET_GONK
        char propValue[PROPERTY_VALUE_MAX];
        property_get("ro.build.version.sdk", propValue, "0");
        if (atoi(propValue) < 15)
            gUseBackingSurface = false;
#endif

        bool current = MakeCurrent();
        if (!current) {
            gfx::LogFailure(NS_LITERAL_CSTRING(
                "Couldn't get device attachments for device."));
            return false;
        }

        SetupLookupFunction();
        if (!InitWithPrefix("gl", true))
            return false;

        PR_STATIC_ASSERT(sizeof(GLint) >= sizeof(int32_t));
        mMaxTextureImageSize = INT32_MAX;

        mShareWithEGLImage = sEGLLibrary.HasKHRImageBase() &&
                             sEGLLibrary.HasKHRImageTexture2D() &&
                             IsExtensionSupported(OES_EGL_image);

        return true;
    }

    bool IsDoubleBuffered() {
        return mIsDoubleBuffered;
    }

    void SetIsDoubleBuffered(bool aIsDB) {
        mIsDoubleBuffered = aIsDB;
    }

    virtual EGLContext GetEGLContext() {
        return mContext;
    }

    virtual GLLibraryEGL* GetLibraryEGL() {
        return &sEGLLibrary;
    }


    bool SupportsRobustness()
    {
        return sEGLLibrary.HasRobustness();
    }

    virtual bool IsANGLE()
    {
        return sEGLLibrary.IsANGLE();
    }

    bool BindTexImage()
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

    bool ReleaseTexImage()
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

    bool BindExternalBuffer(GLuint texture, void* buffer)
    {
#if defined(MOZ_WIDGET_GONK)
        EGLint attrs[] = {
            LOCAL_EGL_IMAGE_PRESERVED, LOCAL_EGL_TRUE,
            LOCAL_EGL_NONE, LOCAL_EGL_NONE
        };
        EGLImage image = sEGLLibrary.fCreateImage(EGL_DISPLAY(),
                                                  EGL_NO_CONTEXT,
                                                  LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                                                  buffer, attrs);
        fBindTexture(LOCAL_GL_TEXTURE_EXTERNAL, texture);
        fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_EXTERNAL, image);
        sEGLLibrary.fDestroyImage(EGL_DISPLAY(), image);
        return true;
#else
        return false;
#endif
    }

    bool UnbindExternalBuffer(GLuint texture)
    {
#if defined(MOZ_WIDGET_GONK)
        fActiveTexture(LOCAL_GL_TEXTURE0);
        fBindTexture(LOCAL_GL_TEXTURE_2D, texture);
        fTexImage2D(LOCAL_GL_TEXTURE_2D, 0,
                    LOCAL_GL_RGBA,
                    1, 1, 0,
                    LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE,
                    nullptr);
        return true;
#else
        return false;
#endif
    }

#ifdef MOZ_WIDGET_GONK
    virtual already_AddRefed<TextureImage>
    CreateDirectTextureImage(GraphicBuffer* aBuffer, GLenum aWrapMode) MOZ_OVERRIDE;
#endif

    virtual void MakeCurrent_EGLSurface(void* surf) {
        EGLSurface eglSurface = (EGLSurface)surf;
        if (!eglSurface)
            eglSurface = mSurface;

        if (eglSurface == mCurSurface)
            return;

        
        mCurSurface = eglSurface;
        MakeCurrent(true);
    }

    bool MakeCurrentImpl(bool aForce = false) {
        bool succeeded = true;

        
        
        
#ifndef MOZ_WIDGET_QT
        if (!mSurface) {
            
            
            
            succeeded = sEGLLibrary.fMakeCurrent(EGL_DISPLAY(),
                                                 EGL_NO_SURFACE, EGL_NO_SURFACE,
                                                 EGL_NO_CONTEXT);
            if (!succeeded && sEGLLibrary.fGetError() == LOCAL_EGL_CONTEXT_LOST) {
                mContextLost = true;
                NS_WARNING("EGL context has been lost.");
            }
            NS_ASSERTION(succeeded, "Failed to make GL context current!");
            return succeeded;
        }
#endif
        if (aForce || sEGLLibrary.fGetCurrentContext() != mContext) {
#ifdef MOZ_WIDGET_QT
            
            if (mSharedContext) {
                QGLContext* qglCtx = static_cast<QGLContext*>(static_cast<GLContextEGL*>(mSharedContext.get())->mPlatformContext);
                if (qglCtx) {
                    qglCtx->doneCurrent();
                }
            }
#endif
            succeeded = sEGLLibrary.fMakeCurrent(EGL_DISPLAY(),
                                                 mCurSurface, mCurSurface,
                                                 mContext);
            
            int eglError = sEGLLibrary.fGetError();
            if (!succeeded) {
                if (eglError == LOCAL_EGL_CONTEXT_LOST) {
                    mContextLost = true;
                    NS_WARNING("EGL context has been lost.");
                } else {
                    NS_WARNING("Failed to make GL context current!");
#ifdef DEBUG
                    printf_stderr("EGL Error: 0x%04x\n", eglError);
#endif
                }
            }
        }

        return succeeded;
    }

    virtual bool IsCurrent() {
        return sEGLLibrary.fGetCurrentContext() == mContext;
    }

#ifdef MOZ_WIDGET_QT
    virtual bool
    RenewSurface() {
        
        return false;
    }
#else
    virtual bool
    RenewSurface() {
        sEGLLibrary.fMakeCurrent(EGL_DISPLAY(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                                 EGL_NO_CONTEXT);
        if (!mSurface) {
#ifdef MOZ_ANDROID_OMTC
            mSurface = mozilla::AndroidBridge::Bridge()->ProvideEGLSurface();
            if (!mSurface) {
                return false;
            }
#else
            EGLConfig config;
            CreateConfig(&config);
            mSurface = CreateSurfaceForWindow(NULL, config);
#endif
        }
        return sEGLLibrary.fMakeCurrent(EGL_DISPLAY(),
                                        mSurface, mSurface,
                                        mContext);
    }
#endif

    virtual void
    ReleaseSurface() {
        if (mSurface && !mPlatformContext) {
            sEGLLibrary.fMakeCurrent(EGL_DISPLAY(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                                     EGL_NO_CONTEXT);
            sEGLLibrary.fDestroySurface(EGL_DISPLAY(), mSurface);
            mSurface = NULL;
        }
    }

    bool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sEGLLibrary.mSymbols.fGetProcAddress;
        return true;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch (aType) {
        case NativeGLContext:
            return mContext;

        default:
            return nullptr;
        }
    }

    bool SwapBuffers()
    {
        if (mSurface && !mPlatformContext) {
#ifdef MOZ_WIDGET_GONK
            if (mHwc)
                return !mHwc->swapBuffers((hwc_display_t)EGL_DISPLAY(),
                                          (hwc_surface_t)mSurface);
            else
#endif
                return sEGLLibrary.fSwapBuffers(EGL_DISPLAY(), mSurface);
        } else {
            return false;
        }
    }
    
    virtual already_AddRefed<TextureImage>
    CreateTextureImage(const nsIntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLenum aWrapMode,
                       TextureImage::Flags aFlags = TextureImage::NoFlags);

    
    virtual already_AddRefed<TextureImage>
    TileGenFunc(const nsIntSize& aSize,
                TextureImage::ContentType aContentType,
                TextureImage::Flags aFlags = TextureImage::NoFlags);
    
    
    void HoldSurface(gfxASurface *aSurf) {
        mThebesSurface = aSurf;
    }

    void SetPlatformContext(void *context) {
        mPlatformContext = context;
    }

    EGLContext Context() {
        return mContext;
    }

    bool BindTex2DOffscreen(GLContext *aOffscreen);
    void UnbindTex2DOffscreen(GLContext *aOffscreen);
    bool ResizeOffscreen(const gfxIntSize& aNewSize);
    void BindOffscreenFramebuffer();

    static already_AddRefed<GLContextEGL>
    CreateEGLPixmapOffscreenContext(const gfxIntSize& size);

    static already_AddRefed<GLContextEGL>
    CreateEGLPBufferOffscreenContext(const gfxIntSize& size);

    virtual bool HasLockSurface() {
        return sEGLLibrary.HasKHRLockSurface();
    }

    virtual SharedTextureHandle CreateSharedHandle(SharedTextureShareType shareType);
    virtual SharedTextureHandle CreateSharedHandle(SharedTextureShareType shareType,
                                                   void* buffer,
                                                   SharedTextureBufferType bufferType);
    virtual void UpdateSharedHandle(SharedTextureShareType shareType,
                                    SharedTextureHandle sharedHandle);
    virtual void ReleaseSharedHandle(SharedTextureShareType shareType,
                                     SharedTextureHandle sharedHandle);
    virtual bool GetSharedHandleDetails(SharedTextureShareType shareType,
                                        SharedTextureHandle sharedHandle,
                                        SharedHandleDetails& details);
    virtual bool AttachSharedHandle(SharedTextureShareType shareType,
                                    SharedTextureHandle sharedHandle);

protected:
    friend class GLContextProviderEGL;

    EGLConfig  mConfig;
    EGLSurface mSurface;
    EGLSurface mCurSurface;
    EGLContext mContext;
    void *mPlatformContext;
    nsRefPtr<gfxASurface> mThebesSurface;
    bool mBound;

    bool mIsPBuffer;
    bool mIsDoubleBuffered;
    bool mCanBindToTexture;
    bool mShareWithEGLImage;
#ifdef MOZ_WIDGET_GONK
    nsRefPtr<HwcComposer2D> mHwc;
#endif

    
    
    GLuint mTemporaryEGLImageTexture;

    static EGLSurface CreatePBufferSurfaceTryingPowerOfTwo(EGLConfig config,
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

        pbattrs.AppendElement(LOCAL_EGL_NONE);

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
};


typedef enum {
    Image
#ifdef MOZ_WIDGET_ANDROID
    , SurfaceTexture
#endif
} SharedHandleType;

class SharedTextureHandleWrapper
{
public:
    SharedTextureHandleWrapper(SharedHandleType aHandleType) : mHandleType(aHandleType)
    {
    }

    virtual ~SharedTextureHandleWrapper()
    {
    }

    SharedHandleType Type() { return mHandleType; }

    SharedHandleType mHandleType;
};

#ifdef MOZ_WIDGET_ANDROID

class SurfaceTextureWrapper: public SharedTextureHandleWrapper
{
public:
    SurfaceTextureWrapper(nsSurfaceTexture* aSurfaceTexture) :
        SharedTextureHandleWrapper(SharedHandleType::SurfaceTexture)
        , mSurfaceTexture(aSurfaceTexture)
    {
    }

    virtual ~SurfaceTextureWrapper() {
        mSurfaceTexture = nullptr;
    }

    nsSurfaceTexture* SurfaceTexture() { return mSurfaceTexture; }

    nsRefPtr<nsSurfaceTexture> mSurfaceTexture;
};

#endif 

class EGLTextureWrapper : public SharedTextureHandleWrapper
{
public:
    EGLTextureWrapper() :
        SharedTextureHandleWrapper(SharedHandleType::Image)
        , mEGLImage(nullptr)
        , mSyncObject(nullptr)
    {
    }

    
    
    
    bool CreateEGLImage(GLContextEGL *ctx, GLuint texture) {
        MOZ_ASSERT(!mEGLImage && texture && sEGLLibrary.HasKHRImageBase());
        static const EGLint eglAttributes[] = {
            LOCAL_EGL_NONE
        };
        EGLContext eglContext = (EGLContext)ctx->GetEGLContext();
        mEGLImage = sEGLLibrary.fCreateImage(EGL_DISPLAY(), eglContext, LOCAL_EGL_GL_TEXTURE_2D,
                                             (EGLClientBuffer)texture, eglAttributes);
        if (!mEGLImage) {
#ifdef DEBUG
            printf_stderr("Could not create EGL images: ERROR (0x%04x)\n", sEGLLibrary.fGetError());
#endif
            return false;
        }
        return true;
    }

    virtual ~EGLTextureWrapper() {
        if (mEGLImage) {
            sEGLLibrary.fDestroyImage(EGL_DISPLAY(), mEGLImage);
            mEGLImage = nullptr;
        }
    }

    const EGLImage GetEGLImage() {
        return mEGLImage;
    }

    
    
    bool MakeSync(GLContext *ctx) {
        MOZ_ASSERT(mSyncObject == nullptr);

        if (sEGLLibrary.IsExtensionSupported(GLLibraryEGL::KHR_fence_sync)) {
            mSyncObject = sEGLLibrary.fCreateSync(EGL_DISPLAY(), LOCAL_EGL_SYNC_FENCE, nullptr);
            
            
            
            ctx->fFlush();
        }

        if (mSyncObject == EGL_NO_SYNC) {
            
            ctx->fFinish();
        }

        return true;
    }

    bool WaitSync() {
        if (!mSyncObject) {
            
            return true;
        }

        
        const uint64_t ns_per_ms = 1000 * 1000;
        EGLTime timeout = 1000 * ns_per_ms;

        EGLint result = sEGLLibrary.fClientWaitSync(EGL_DISPLAY(), mSyncObject, 0, timeout);
        sEGLLibrary.fDestroySync(EGL_DISPLAY(), mSyncObject);
        mSyncObject = nullptr;

        return result == LOCAL_EGL_CONDITION_SATISFIED;
    }

private:
    EGLImage mEGLImage;
    EGLSync mSyncObject;
};

void
GLContextEGL::UpdateSharedHandle(SharedTextureShareType shareType,
                                 SharedTextureHandle sharedHandle)
{
    if (shareType != SameProcess) {
        NS_ERROR("Implementation not available for this sharing type");
        return;
    }

    SharedTextureHandleWrapper* wrapper = reinterpret_cast<SharedTextureHandleWrapper*>(sharedHandle);

    NS_ASSERTION(wrapper->Type() == SharedHandleType::Image, "Expected EGLImage shared handle");
    NS_ASSERTION(mShareWithEGLImage, "EGLImage not supported or disabled in runtime");

    EGLTextureWrapper* wrap = reinterpret_cast<EGLTextureWrapper*>(wrapper);
    
    
    
    ScopedBindTexture autoTex(this, mTemporaryEGLImageTexture);
    fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, wrap->GetEGLImage());

    
    
    
    gfxIntSize size = OffscreenSize();
    BlitFramebufferToTexture(0, mTemporaryEGLImageTexture, size, size);

    
    
    
    wrap->MakeSync(this);
}

SharedTextureHandle
GLContextEGL::CreateSharedHandle(SharedTextureShareType shareType)
{
    if (shareType != SameProcess)
        return 0;

    if (!mShareWithEGLImage)
        return 0;

    MakeCurrent();
    mTemporaryEGLImageTexture = CreateTextureForOffscreen(GetGLFormats(), OffscreenSize());

    EGLTextureWrapper* tex = new EGLTextureWrapper();
    bool ok = tex->CreateEGLImage(this, mTemporaryEGLImageTexture);

    if (!ok) {
        NS_ERROR("EGLImage creation for EGLTextureWrapper failed");
        ReleaseSharedHandle(shareType, (SharedTextureHandle)tex);
        return 0;
    }

    
    return (SharedTextureHandle)tex;
}

SharedTextureHandle
GLContextEGL::CreateSharedHandle(SharedTextureShareType shareType,
                                 void* buffer,
                                 SharedTextureBufferType bufferType)
{
    
    
    if (shareType != SameProcess)
        return 0;

    switch (bufferType) {
#ifdef MOZ_WIDGET_ANDROID
    case SharedTextureBufferType::SurfaceTexture:
        if (!IsExtensionSupported(GLContext::OES_EGL_image_external)) {
            NS_WARNING("Missing GL_OES_EGL_image_external");
            return 0;
        }

        return (SharedTextureHandle) new SurfaceTextureWrapper(reinterpret_cast<nsSurfaceTexture*>(buffer));
#endif
    case SharedTextureBufferType::TextureID: {
        if (!mShareWithEGLImage)
            return 0;

        GLuint texture = (uintptr_t)buffer;
        EGLTextureWrapper* tex = new EGLTextureWrapper();
        if (!tex->CreateEGLImage(this, texture)) {
            NS_ERROR("EGLImage creation for EGLTextureWrapper failed");
            delete tex;
            return 0;
        }

        return (SharedTextureHandle)tex;
    }
    default:
        NS_ERROR("Unknown shared texture buffer type");
        return 0;
    }
}

void GLContextEGL::ReleaseSharedHandle(SharedTextureShareType shareType,
                                       SharedTextureHandle sharedHandle)
{
    if (shareType != SameProcess) {
        NS_ERROR("Implementation not available for this sharing type");
        return;
    }

    SharedTextureHandleWrapper* wrapper = reinterpret_cast<SharedTextureHandleWrapper*>(sharedHandle);

    switch (wrapper->Type()) {
#ifdef MOZ_WIDGET_ANDROID
    case SharedHandleType::SurfaceTexture:
        delete wrapper;
        break;
#endif
    
    case SharedHandleType::Image: {
        NS_ASSERTION(mShareWithEGLImage, "EGLImage not supported or disabled in runtime");

        EGLTextureWrapper* wrap = (EGLTextureWrapper*)sharedHandle;
        delete wrap;
        break;
    }

    default:
        NS_ERROR("Unknown shared handle type");
        return;
    }
}

bool GLContextEGL::GetSharedHandleDetails(SharedTextureShareType shareType,
                                          SharedTextureHandle sharedHandle,
                                          SharedHandleDetails& details)
{
    if (shareType != SameProcess)
        return false;

    SharedTextureHandleWrapper* wrapper = reinterpret_cast<SharedTextureHandleWrapper*>(sharedHandle);

    switch (wrapper->Type()) {
#ifdef MOZ_WIDGET_ANDROID
    case SharedHandleType::SurfaceTexture: {
        SurfaceTextureWrapper* surfaceWrapper = reinterpret_cast<SurfaceTextureWrapper*>(wrapper);

        details.mTarget = LOCAL_GL_TEXTURE_EXTERNAL;
        details.mProgramType = RGBALayerExternalProgramType;
        surfaceWrapper->SurfaceTexture()->GetTransformMatrix(details.mTextureTransform);
        break;
    }
#endif

    case SharedHandleType::Image:
        details.mTarget = LOCAL_GL_TEXTURE_2D;
        details.mProgramType = RGBALayerProgramType;
        break;

    default:
        NS_ERROR("Unknown shared handle type");
        return false;
    }

    return true;
}

bool GLContextEGL::AttachSharedHandle(SharedTextureShareType shareType,
                                      SharedTextureHandle sharedHandle)
{
    if (shareType != SameProcess)
        return false;

    SharedTextureHandleWrapper* wrapper = reinterpret_cast<SharedTextureHandleWrapper*>(sharedHandle);

    switch (wrapper->Type()) {
#ifdef MOZ_WIDGET_ANDROID
    case SharedHandleType::SurfaceTexture: {
#ifndef DEBUG
        



        GetAndClearError();
#endif
        SurfaceTextureWrapper* surfaceTextureWrapper = reinterpret_cast<SurfaceTextureWrapper*>(wrapper);

        
        
        
        surfaceTextureWrapper->SurfaceTexture()->UpdateTexImage();
        break;
    }
#endif 

    case SharedHandleType::Image: {
        NS_ASSERTION(mShareWithEGLImage, "EGLImage not supported or disabled in runtime");

        EGLTextureWrapper* wrap = (EGLTextureWrapper*)sharedHandle;
        wrap->WaitSync();
        fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, wrap->GetEGLImage());
        break;
    }

    default:
        NS_ERROR("Unknown shared handle type");
        return false;
    }

    return true;
}

bool
GLContextEGL::ResizeOffscreen(const gfxIntSize& aNewSize)
{
	return ResizeScreenBuffer(aNewSize);
}


static GLContextEGL *
GetGlobalContextEGL()
{
    return static_cast<GLContextEGL*>(GLContextProviderEGL::GetGlobalContext());
}

static GLenum
GLFormatForImage(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxASurface::ImageFormatARGB32:
    case gfxASurface::ImageFormatRGB24:
        
        return LOCAL_GL_RGBA;
    case gfxASurface::ImageFormatRGB16_565:
        return LOCAL_GL_RGB;
    case gfxASurface::ImageFormatA8:
        return LOCAL_GL_LUMINANCE;
    default:
        NS_WARNING("Unknown GL format for Image format");
    }
    return 0;
}

#ifdef MOZ_WIDGET_GONK
static PixelFormat
PixelFormatForImage(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxASurface::ImageFormatARGB32:
        return PIXEL_FORMAT_RGBA_8888;
    case gfxASurface::ImageFormatRGB24:
        return PIXEL_FORMAT_RGBX_8888;
    case gfxASurface::ImageFormatRGB16_565:
        return PIXEL_FORMAT_RGB_565;
    case gfxASurface::ImageFormatA8:
        return PIXEL_FORMAT_L_8;
    default:
        MOZ_NOT_REACHED("Unknown gralloc pixel format for Image format");
    }
    return 0;
}

static gfxASurface::gfxContentType
ContentTypeForPixelFormat(PixelFormat aFormat)
{
    switch (aFormat) {
    case PIXEL_FORMAT_L_8:
        return gfxASurface::CONTENT_ALPHA;
    case PIXEL_FORMAT_RGBA_8888:
        return gfxASurface::CONTENT_COLOR_ALPHA;
    case PIXEL_FORMAT_RGBX_8888:
    case PIXEL_FORMAT_RGB_565:
        return gfxASurface::CONTENT_COLOR;
    default:
        MOZ_NOT_REACHED("Unknown content type for gralloc pixel format");
    }
    return gfxASurface::CONTENT_COLOR;
}
#endif

static GLenum
GLTypeForImage(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxASurface::ImageFormatARGB32:
    case gfxASurface::ImageFormatRGB24:
    case gfxASurface::ImageFormatA8:
        return LOCAL_GL_UNSIGNED_BYTE;
    case gfxASurface::ImageFormatRGB16_565:
        return LOCAL_GL_UNSIGNED_SHORT_5_6_5;
    default:
        NS_WARNING("Unknown GL format for Image format");
    }
    return 0;
}

class TextureImageEGL
    : public TextureImage
{
public:
    TextureImageEGL(GLuint aTexture,
                    const nsIntSize& aSize,
                    GLenum aWrapMode,
                    ContentType aContentType,
                    GLContext* aContext,
                    Flags aFlags = TextureImage::NoFlags,
                    TextureState aTextureState = Created)
        : TextureImage(aSize, aWrapMode, aContentType, aFlags)
        , mGLContext(aContext)
        , mUpdateFormat(gfxASurface::ImageFormatUnknown)
        , mEGLImage(nullptr)
        , mTexture(aTexture)
        , mSurface(nullptr)
        , mConfig(nullptr)
        , mTextureState(aTextureState)
        , mBound(false)
        , mIsLocked(false)
    {
        mUpdateFormat = gfxPlatform::GetPlatform()->OptimalFormatForContent(GetContentType());

        if (gUseBackingSurface) {
#ifdef MOZ_WIDGET_GONK
            switch (mUpdateFormat) {
            case gfxASurface::ImageFormatARGB32:
                mShaderType = BGRALayerProgramType;
                break;
            case gfxASurface::ImageFormatRGB24:
                mUpdateFormat = gfxASurface::ImageFormatARGB32;
                mShaderType = BGRXLayerProgramType;
                break;
            case gfxASurface::ImageFormatRGB16_565:
                mShaderType = RGBXLayerProgramType;
                break;
            case gfxASurface::ImageFormatA8:
                mShaderType = RGBALayerProgramType;
                break;
            default:
                MOZ_NOT_REACHED("Unknown update format");
            }
#else
            if (mUpdateFormat != gfxASurface::ImageFormatARGB32) {
                mShaderType = RGBXLayerProgramType;
            } else {
                mShaderType = RGBALayerProgramType;
            }
#endif
            Resize(aSize);
        } else {
            if (mUpdateFormat == gfxASurface::ImageFormatRGB16_565) {
                mShaderType = RGBXLayerProgramType;
            } else if (mUpdateFormat == gfxASurface::ImageFormatRGB24) {
                
                
                
                mShaderType = BGRXLayerProgramType;
            } else {
                mShaderType = BGRALayerProgramType;
            }
        }
    }

    virtual ~TextureImageEGL()
    {
        GLContext *ctx = mGLContext;
        if (ctx->IsDestroyed() || !ctx->IsOwningThreadCurrent()) {
            ctx = ctx->GetSharedContext();
        }

        
        
        
        
        if (ctx && !ctx->IsDestroyed()) {
            ctx->MakeCurrent();
            ctx->fDeleteTextures(1, &mTexture);
            ReleaseTexImage();
            DestroyEGLSurface();
        }
    }

    bool UsingDirectTexture()
    {
#ifdef MOZ_WIDGET_GONK
        if (mGraphicBuffer != nullptr)
            return true;
#endif
        return !!mBackingSurface;
    }

    virtual void GetUpdateRegion(nsIntRegion& aForRegion)
    {
        if (mTextureState != Valid) {
            
            
            aForRegion = nsIntRect(nsIntPoint(0, 0), mSize);
        }

        if (UsingDirectTexture()) {
            return;
        }

        
        
        
        
        aForRegion = nsIntRegion(aForRegion.GetBounds());
    }

    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion)
    {
        NS_ASSERTION(!mUpdateSurface, "BeginUpdate() without EndUpdate()?");

        
        GetUpdateRegion(aRegion);
        mUpdateRect = aRegion.GetBounds();

        
        if (!nsIntRect(nsIntPoint(0, 0), mSize).Contains(mUpdateRect)) {
            NS_ERROR("update outside of image");
            return NULL;
        }

#ifdef MOZ_WIDGET_GONK
        if (mGraphicBuffer != nullptr) {
            mUpdateSurface = GetLockSurface();

            return mUpdateSurface;
        }
#endif

        if (mBackingSurface) {
            if (sEGLLibrary.HasKHRLockSurface()) {
                mUpdateSurface = GetLockSurface();
            } else {
                mUpdateSurface = mBackingSurface;
            }

            return mUpdateSurface;
        }

        

        mUpdateSurface =
            new gfxImageSurface(gfxIntSize(mUpdateRect.width, mUpdateRect.height),
                                mUpdateFormat);

        mUpdateSurface->SetDeviceOffset(gfxPoint(-mUpdateRect.x, -mUpdateRect.y));

        return mUpdateSurface;
    }

    virtual void EndUpdate()
    {
        NS_ASSERTION(!!mUpdateSurface, "EndUpdate() without BeginUpdate()?");

        if (mIsLocked) {
            UnlockSurface();
            mTextureState = Valid;
            mUpdateSurface = nullptr;
            return;
        }

        if (mBackingSurface && mUpdateSurface == mBackingSurface) {
#ifdef MOZ_X11
            if (mBackingSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
                FinishX(DefaultXDisplay());
            }
#endif

            mBackingSurface->SetDeviceOffset(gfxPoint(0, 0));
            mTextureState = Valid;
            mUpdateSurface = nullptr;
            return;
        }

        

        
        
        

        
        
        
        mUpdateSurface->SetDeviceOffset(gfxPoint(0, 0));

        nsRefPtr<gfxImageSurface> uploadImage = nullptr;
        gfxIntSize updateSize(mUpdateRect.width, mUpdateRect.height);

        NS_ASSERTION(mUpdateSurface->GetType() == gfxASurface::SurfaceTypeImage &&
                     mUpdateSurface->GetSize() == updateSize,
                     "Upload image isn't an image surface when one is expected, or is wrong size!");

        uploadImage = static_cast<gfxImageSurface*>(mUpdateSurface.get());

        if (!uploadImage) {
            return;
        }

        mGLContext->MakeCurrent();
        mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

        if (mTextureState != Valid) {
            NS_ASSERTION(mUpdateRect.x == 0 && mUpdateRect.y == 0 &&
                         mUpdateRect.Size() == mSize,
                         "Bad initial update on non-created texture!");

            mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                                    0,
                                    GLFormatForImage(mUpdateFormat),
                                    mUpdateRect.width,
                                    mUpdateRect.height,
                                    0,
                                    GLFormatForImage(uploadImage->Format()),
                                    GLTypeForImage(uploadImage->Format()),
                                    uploadImage->Data());
        } else {
            mGLContext->fTexSubImage2D(LOCAL_GL_TEXTURE_2D,
                                       0,
                                       mUpdateRect.x,
                                       mUpdateRect.y,
                                       mUpdateRect.width,
                                       mUpdateRect.height,
                                       GLFormatForImage(uploadImage->Format()),
                                       GLTypeForImage(uploadImage->Format()),
                                       uploadImage->Data());
        }

        mUpdateSurface = nullptr;
        mTextureState = Valid;
        return;         
    }

    virtual bool DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom )
    {
        nsIntRect bounds = aRegion.GetBounds();

        nsIntRegion region;
        if (mTextureState != Valid) {
            bounds = nsIntRect(0, 0, mSize.width, mSize.height);
            region = nsIntRegion(bounds);
        } else {
            region = aRegion;
        }

        if ((mBackingSurface && sEGLLibrary.HasKHRLockSurface())
#ifdef MOZ_WIDGET_GONK
            || (mGraphicBuffer != nullptr)
#endif
            ) {
            mUpdateSurface = GetLockSurface();
            if (mUpdateSurface) {
                nsRefPtr<gfxContext> ctx = new gfxContext(mUpdateSurface);
                gfxUtils::ClipToRegion(ctx, aRegion);
                ctx->SetSource(aSurf, gfxPoint(-aFrom.x, -aFrom.y));
                ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
                ctx->Paint();
                mUpdateSurface = nullptr;
                UnlockSurface();
            }
        } else {
            mShaderType =
              mGLContext->UploadSurfaceToTexture(aSurf,
                                                 region,
                                                 mTexture,
                                                 mTextureState == Created,
                                                 bounds.TopLeft() + aFrom,
                                                 false);
        }

        mTextureState = Valid;
        return true;
    }

    virtual void BindTexture(GLenum aTextureUnit)
    {
        
        if (mTextureState == Created) {
            Resize(mSize);
        }

#ifdef MOZ_WIDGET_GONK
        if (UsingDirectTexture()) {
            mGLContext->fActiveTexture(aTextureUnit);
            mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
            mGLContext->fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, mEGLImage);
            if (sEGLLibrary.fGetError() != LOCAL_EGL_SUCCESS) {
               LOG("Could not set image target texture. ERROR (0x%04x)", sEGLLibrary.fGetError());
            }
        } else
#endif
        {
            mGLContext->fActiveTexture(aTextureUnit);
            mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
            mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
        }
    }

    virtual GLuint GetTextureID() 
    {
        
        if (mTextureState == Created) {
            Resize(mSize);
        }
        return mTexture;
    };

    virtual bool InUpdate() const { return !!mUpdateSurface; }

    virtual void Resize(const nsIntSize& aSize)
    {
        NS_ASSERTION(!mUpdateSurface, "Resize() while in update?");

        if (mSize == aSize && mTextureState != Created)
            return;

        mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    
        
        if (gUseBackingSurface) {
            CreateBackingSurface(gfxIntSize(aSize.width, aSize.height));
        }

        if (!UsingDirectTexture()) {
            
            
            mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                                    0,
                                    GLFormatForImage(mUpdateFormat),
                                    aSize.width,
                                    aSize.height,
                                    0,
                                    GLFormatForImage(mUpdateFormat),
                                    GLTypeForImage(mUpdateFormat),
                                    NULL);
        }

        mTextureState = Allocated;
        mSize = aSize;
    }

    bool BindTexImage()
    {
        if (mBound && !ReleaseTexImage())
            return false;

        EGLBoolean success =
            sEGLLibrary.fBindTexImage(EGL_DISPLAY(),
                                      (EGLSurface)mSurface,
                                      LOCAL_EGL_BACK_BUFFER);

        if (success == LOCAL_EGL_FALSE)
            return false;

        mBound = true;
        return true;
    }

    bool ReleaseTexImage()
    {
        if (!mBound)
            return true;

        EGLBoolean success =
            sEGLLibrary.fReleaseTexImage(EGL_DISPLAY(),
                                         (EGLSurface)mSurface,
                                         LOCAL_EGL_BACK_BUFFER);

        if (success == LOCAL_EGL_FALSE)
            return false;

        mBound = false;
        return true;
    }

    virtual already_AddRefed<gfxImageSurface> GetLockSurface()
    {
        if (mIsLocked) {
            NS_WARNING("Can't lock surface twice");
            return nullptr;
        }

#ifdef MOZ_WIDGET_GONK
        if (mGraphicBuffer != nullptr) {
            
            mGLContext->MakeCurrent(true);
            mGLContext->UnbindExternalBuffer(mTexture);

            void *vaddr;
            if (mGraphicBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN |
                                     GraphicBuffer::USAGE_SW_WRITE_OFTEN,
                                     &vaddr) != OK) {
                LOG("Could not lock GraphicBuffer");
                return nullptr;
            }

            nsRefPtr<gfxImageSurface> surface =
                new gfxImageSurface(reinterpret_cast<unsigned char *>(vaddr),
                                    gfxIntSize(mSize.width, mSize.height),
                                    mGraphicBuffer->getStride() * gfxUtils::ImageFormatToDepth(mUpdateFormat) / 8,
                                    mUpdateFormat);

            mIsLocked = true;

            return surface.forget();
        }
#endif

        if (!sEGLLibrary.HasKHRLockSurface()) {
            NS_WARNING("GetLockSurface called, but no EGL_KHR_lock_surface extension!");
            return nullptr;
        }

        if (!CreateEGLSurface(mBackingSurface)) {
            NS_WARNING("Failed to create EGL surface");
            return nullptr;
        }

        static EGLint lock_attribs[] = {
            LOCAL_EGL_MAP_PRESERVE_PIXELS_KHR, LOCAL_EGL_TRUE,
            LOCAL_EGL_LOCK_USAGE_HINT_KHR, LOCAL_EGL_READ_SURFACE_BIT_KHR | LOCAL_EGL_WRITE_SURFACE_BIT_KHR,
            LOCAL_EGL_NONE
        };

        sEGLLibrary.fLockSurface(EGL_DISPLAY(), mSurface, lock_attribs);

        mIsLocked = true;

        unsigned char *data = nullptr;
        int pitch = 0;
        int pixsize = 0;

        sEGLLibrary.fQuerySurface(EGL_DISPLAY(), mSurface, LOCAL_EGL_BITMAP_POINTER_KHR, (EGLint*)&data);
        sEGLLibrary.fQuerySurface(EGL_DISPLAY(), mSurface, LOCAL_EGL_BITMAP_PITCH_KHR, &pitch);
        sEGLLibrary.fQuerySurface(EGL_DISPLAY(), mSurface, LOCAL_EGL_BITMAP_PIXEL_SIZE_KHR, &pixsize);

        nsRefPtr<gfxImageSurface> sharedImage =
            new gfxImageSurface(data,
                                mBackingSurface->GetSize(),
                                pitch,
                                mUpdateFormat);

        return sharedImage.forget();
    }

    virtual void UnlockSurface()
    {
        if (!mIsLocked) {
            NS_WARNING("UnlockSurface called, surface not locked!");
            return;
        }

        mIsLocked = false;

#ifdef MOZ_WIDGET_GONK
        if (mGraphicBuffer != nullptr) {
            mGraphicBuffer->unlock();

            return;
        }
#endif

        sEGLLibrary.fUnlockSurface(EGL_DISPLAY(), mSurface);
    }

    virtual already_AddRefed<gfxASurface> GetBackingSurface()
    {
        nsRefPtr<gfxASurface> copy = mBackingSurface;
        return copy.forget();
    }

    virtual bool CreateEGLSurface(gfxASurface* aSurface)
    {
#ifdef MOZ_X11
        if (!aSurface) {
            NS_WARNING("no surface");
            return false;
        }

        if (aSurface->GetType() != gfxASurface::SurfaceTypeXlib) {
            NS_WARNING("wrong surface type, must be xlib");
            return false;
        }

        if (mSurface) {
            return true;
        }

        EGLSurface surface = CreateEGLSurfaceForXSurface(aSurface, &mConfig);

        if (!surface) {
            NS_WARNING("couldn't find X config for surface");
            return false;
        }

        mSurface = surface;
        return true;
#else
        return false;
#endif
    }

    virtual void DestroyEGLSurface(void)
    {
#ifdef MOZ_WIDGET_GONK
        mGraphicBuffer.clear();

        if (mEGLImage) {
            sEGLLibrary.fDestroyImage(EGL_DISPLAY(), mEGLImage);
            mEGLImage = nullptr;
        }
#endif

        if (!mSurface)
            return;

        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), mSurface);
        mSurface = nullptr;
    }

    virtual bool CreateBackingSurface(const gfxIntSize& aSize)
    {
        ReleaseTexImage();
        DestroyEGLSurface();
        mBackingSurface = nullptr;

#ifdef MOZ_X11
        Display* dpy = DefaultXDisplay();
        XRenderPictFormat* renderFMT =
            gfxXlibSurface::FindRenderFormat(dpy, mUpdateFormat);

        nsRefPtr<gfxXlibSurface> xsurface =
            gfxXlibSurface::Create(DefaultScreenOfDisplay(dpy),
                                   renderFMT,
                                   gfxIntSize(aSize.width, aSize.height));

        XSync(dpy, False);
        mConfig = nullptr;

        if (sEGLLibrary.HasKHRImagePixmap() &&
            mGLContext->IsExtensionSupported(GLContext::OES_EGL_image))
        {
            mEGLImage =
                sEGLLibrary.fCreateImage(EGL_DISPLAY(),
                                         EGL_NO_CONTEXT,
                                         LOCAL_EGL_NATIVE_PIXMAP,
                                         (EGLClientBuffer)xsurface->XDrawable(),
                                         nullptr);

            if (!mEGLImage) {
                printf_stderr("couldn't create EGL image: ERROR (0x%04x)\n", sEGLLibrary.fGetError());
                return false;
            }
            mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
            mGLContext->fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, mEGLImage);
            sEGLLibrary.fDestroyImage(EGL_DISPLAY(), mEGLImage);
            mEGLImage = nullptr;
        } else {
            if (!CreateEGLSurface(xsurface)) {
                printf_stderr("ProviderEGL Failed create EGL surface: ERROR (0x%04x)\n", sEGLLibrary.fGetError());
                return false;
            }

            if (!BindTexImage()) {
                printf_stderr("ProviderEGL Failed to bind teximage: ERROR (0x%04x)\n", sEGLLibrary.fGetError());
                return false;
            }
        }

        mBackingSurface = xsurface;

        return mBackingSurface != nullptr;
#endif

#ifdef MOZ_WIDGET_GONK
        if (gUseBackingSurface && aSize.width >= 64) {
            mGLContext->MakeCurrent(true);
            PixelFormat format = PixelFormatForImage(mUpdateFormat);
            uint32_t usage = GraphicBuffer::USAGE_HW_TEXTURE |
                             GraphicBuffer::USAGE_SW_READ_OFTEN |
                             GraphicBuffer::USAGE_SW_WRITE_OFTEN;
            mGraphicBuffer = new GraphicBuffer(aSize.width, aSize.height, format, usage);
            if (mGraphicBuffer->initCheck() == OK) {
                const int eglImageAttributes[] = { LOCAL_EGL_IMAGE_PRESERVED, LOCAL_EGL_TRUE,
                                                   LOCAL_EGL_NONE, LOCAL_EGL_NONE };
                mEGLImage = sEGLLibrary.fCreateImage(EGL_DISPLAY(),
                                                        EGL_NO_CONTEXT,
                                                        LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                                                        (EGLClientBuffer) mGraphicBuffer->getNativeBuffer(),
                                                        eglImageAttributes);
                if (!mEGLImage) {
                    mGraphicBuffer = nullptr;
                    LOG("Could not create EGL images: ERROR (0x%04x)", sEGLLibrary.fGetError());
                    return false;
                }

                return true;
            }

            mGraphicBuffer = nullptr;
            LOG("GraphicBufferAllocator::alloc failed");
            return false;
        }
#endif
        return mBackingSurface != nullptr;
    }

protected:
    typedef gfxASurface::gfxImageFormat ImageFormat;

    GLContext* mGLContext;

    nsIntRect mUpdateRect;
    ImageFormat mUpdateFormat;
    bool mUsingDirectTexture;
    nsRefPtr<gfxASurface> mBackingSurface;
    nsRefPtr<gfxASurface> mUpdateSurface;
#ifdef MOZ_WIDGET_GONK
    sp<GraphicBuffer> mGraphicBuffer;
#endif
    EGLImage mEGLImage;
    GLuint mTexture;
    EGLSurface mSurface;
    EGLConfig mConfig;
    TextureState mTextureState;

    bool mBound;
    bool mIsLocked;

    virtual void ApplyFilter()
    {
        mGLContext->ApplyFilterToBoundTexture(mFilter);
    }
};

#ifdef MOZ_WIDGET_GONK

class DirectTextureImageEGL
    : public TextureImageEGL
{
public:
    DirectTextureImageEGL(GLuint aTexture,
                          sp<GraphicBuffer> aGraphicBuffer,
                          GLenum aWrapMode,
                          GLContext* aContext)
        : TextureImageEGL(aTexture,
                          nsIntSize(aGraphicBuffer->getWidth(), aGraphicBuffer->getHeight()),
                          aWrapMode,
                          ContentTypeForPixelFormat(aGraphicBuffer->getPixelFormat()),
                          aContext,
                          ForceSingleTile,
                          Valid)
    {
        mGraphicBuffer = aGraphicBuffer;

        const int eglImageAttributes[] =
            { LOCAL_EGL_IMAGE_PRESERVED, LOCAL_EGL_TRUE,
              LOCAL_EGL_NONE, LOCAL_EGL_NONE };

        mEGLImage = sEGLLibrary.fCreateImage(EGL_DISPLAY(),
                                             EGL_NO_CONTEXT,
                                             LOCAL_EGL_NATIVE_BUFFER_ANDROID,
                                             mGraphicBuffer->getNativeBuffer(),
                                             eglImageAttributes);
        if (!mEGLImage) {
            LOG("Could not create EGL images: ERROR (0x%04x)", sEGLLibrary.fGetError());
        }
    }
};

#endif  

already_AddRefed<TextureImage>
GLContextEGL::CreateTextureImage(const nsIntSize& aSize,
                                 TextureImage::ContentType aContentType,
                                 GLenum aWrapMode,
                                 TextureImage::Flags aFlags)
{
    nsRefPtr<TextureImage> t = new gl::TiledTextureImage(this, aSize, aContentType, aFlags);
    return t.forget();
}

#ifdef MOZ_WIDGET_GONK
already_AddRefed<TextureImage>
GLContextEGL::CreateDirectTextureImage(GraphicBuffer* aBuffer,
                                       GLenum aWrapMode)
{
    MakeCurrent();

    GLuint texture;
    fGenTextures(1, &texture);

    nsRefPtr<TextureImage> texImage(
        new DirectTextureImageEGL(texture, aBuffer, aWrapMode, this));
    texImage->BindTexture(LOCAL_GL_TEXTURE0);

    GLint texfilter = LOCAL_GL_LINEAR;
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, texfilter);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, texfilter);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, aWrapMode);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, aWrapMode);

    return texImage.forget();
}
#endif  

already_AddRefed<TextureImage>
GLContextEGL::TileGenFunc(const nsIntSize& aSize,
                                 TextureImage::ContentType aContentType,
                                 TextureImage::Flags aFlags)
{
  MakeCurrent();

  GLuint texture;
  fGenTextures(1, &texture);

  nsRefPtr<TextureImageEGL> teximage =
      new TextureImageEGL(texture, aSize, LOCAL_GL_CLAMP_TO_EDGE, aContentType, this, aFlags);
  
  teximage->BindTexture(LOCAL_GL_TEXTURE0);

  GLint texfilter = aFlags & TextureImage::UseNearestFilter ? LOCAL_GL_NEAREST : LOCAL_GL_LINEAR;
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, texfilter);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, texfilter);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

  return teximage.forget();
}

static nsRefPtr<GLContext> gGlobalContext;

static const EGLint kEGLConfigAttribsOffscreenPBuffer[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_PBUFFER_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_NONE
};

static const EGLint kEGLConfigAttribsRGB16[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        5,
    LOCAL_EGL_GREEN_SIZE,      6,
    LOCAL_EGL_BLUE_SIZE,       5,
    LOCAL_EGL_ALPHA_SIZE,      0,
    LOCAL_EGL_NONE
};

static const EGLint kEGLConfigAttribsRGB24[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        8,
    LOCAL_EGL_GREEN_SIZE,      8,
    LOCAL_EGL_BLUE_SIZE,       8,
    LOCAL_EGL_ALPHA_SIZE,      0,
    LOCAL_EGL_NONE
};

static const EGLint kEGLConfigAttribsRGBA32[] = {
    LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
    LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
    LOCAL_EGL_RED_SIZE,        8,
    LOCAL_EGL_GREEN_SIZE,      8,
    LOCAL_EGL_BLUE_SIZE,       8,
    LOCAL_EGL_ALPHA_SIZE,      8,
    LOCAL_EGL_NONE
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
#endif
        return false;
    } else {
        return true;
    }
}



#ifndef MOZ_ANDROID_OMTC
static EGLSurface
CreateSurfaceForWindow(nsIWidget *aWidget, EGLConfig config)
{
    EGLSurface surface;

#ifdef DEBUG
    sEGLLibrary.DumpEGLConfig(config);
#endif

#if !defined(MOZ_WIDGET_ANDROID)
    surface = sEGLLibrary.fCreateWindowSurface(EGL_DISPLAY(), config, GET_NATIVE_WINDOW(aWidget), 0);
#endif

#ifdef MOZ_WIDGET_GONK
    gScreenBounds.x = 0;
    gScreenBounds.y = 0;
    sEGLLibrary.fQuerySurface(EGL_DISPLAY(), surface, LOCAL_EGL_WIDTH, &gScreenBounds.width);
    sEGLLibrary.fQuerySurface(EGL_DISPLAY(), surface, LOCAL_EGL_HEIGHT, &gScreenBounds.height);
#endif

    return surface;
}
#endif

already_AddRefed<GLContext>
GLContextProviderEGL::CreateForWindow(nsIWidget *aWidget)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nullptr;
    }

    bool doubleBuffered = true;

    bool hasNativeContext = aWidget->HasGLContext();
    EGLContext eglContext = sEGLLibrary.fGetCurrentContext();
    if (hasNativeContext && eglContext) {
        void* platformContext = eglContext;
        SurfaceCaps caps = SurfaceCaps::Any();
#ifdef MOZ_WIDGET_QT
        int depth = gfxPlatform::GetPlatform()->GetScreenDepth();
        QGLContext* context = const_cast<QGLContext*>(QGLContext::currentContext());
        if (context && context->device()) {
            depth = context->device()->depth();
        }
        const QGLFormat& format = context->format();
        doubleBuffered = format.doubleBuffer();
        platformContext = context;
        caps.bpp16 = depth == 16 ? true : false;
        caps.alpha = format.rgba();
        caps.depth = format.depth();
        caps.stencil = format.stencil();
#endif
        EGLConfig config = EGL_NO_CONFIG;
        EGLSurface surface = sEGLLibrary.fGetCurrentSurface(LOCAL_EGL_DRAW);
        nsRefPtr<GLContextEGL> glContext =
            new GLContextEGL(caps,
                             gGlobalContext, false,
                             config, surface, eglContext);

        if (!glContext->Init())
            return nullptr;

        glContext->MakeCurrent();
        glContext->SetIsDoubleBuffered(doubleBuffered);
        glContext->SetPlatformContext(platformContext);

        if (!gGlobalContext) {
            gGlobalContext = glContext;
        }

        return glContext.forget();
    }

    EGLConfig config;
    if (!CreateConfig(&config)) {
        printf_stderr("Failed to create EGL config!\n");
        return nullptr;
    }

#ifdef MOZ_ANDROID_OMTC
    mozilla::AndroidBridge::Bridge()->RegisterCompositor();
    EGLSurface surface = mozilla::AndroidBridge::Bridge()->ProvideEGLSurface();
#else
    EGLSurface surface = CreateSurfaceForWindow(aWidget, config);
#endif

    if (!surface) {
        printf_stderr("Failed to create EGLSurface!\n");
        return nullptr;
    }

    GLContextEGL* shareContext = GetGlobalContextEGL();
    SurfaceCaps caps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(caps,
                                      shareContext, false,
                                      config, surface);

    if (!glContext) {
        sEGLLibrary.fDestroySurface(EGL_DISPLAY(), surface);
        return nullptr;
    }

    glContext->MakeCurrent();
    glContext->SetIsDoubleBuffered(doubleBuffered);

    return glContext.forget();
}

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
    if (GLContext::DebugMode())
        sEGLLibrary.DumpEGLConfig(config);

    gfxIntSize pbSize(size);
    surface = GLContextEGL::CreatePBufferSurfaceTryingPowerOfTwo(config,
                                                                 LOCAL_EGL_NONE,
                                                                 pbSize);
    if (!surface) {
        NS_WARNING("Failed to create PBuffer for context!");
        return nullptr;
    }

    GLContextEGL* shareContext = GetGlobalContextEGL();
    SurfaceCaps dummyCaps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(dummyCaps,
                                      shareContext, true,
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

#ifdef MOZ_X11
EGLSurface
CreateEGLSurfaceForXSurface(gfxASurface* aSurface, EGLConfig* aConfig)
{
    gfxXlibSurface* xsurface = static_cast<gfxXlibSurface*>(aSurface);
    bool opaque =
        aSurface->GetContentType() == gfxASurface::CONTENT_COLOR;

    static EGLint pixmap_config_rgb[] = {
        LOCAL_EGL_TEXTURE_TARGET,       LOCAL_EGL_TEXTURE_2D,
        LOCAL_EGL_TEXTURE_FORMAT,       LOCAL_EGL_TEXTURE_RGB,
        LOCAL_EGL_NONE
    };

    static EGLint pixmap_config_rgba[] = {
        LOCAL_EGL_TEXTURE_TARGET,       LOCAL_EGL_TEXTURE_2D,
        LOCAL_EGL_TEXTURE_FORMAT,       LOCAL_EGL_TEXTURE_RGBA,
        LOCAL_EGL_NONE
    };

    EGLSurface surface = nullptr;
    if (aConfig && *aConfig) {
        if (opaque)
            surface = sEGLLibrary.fCreatePixmapSurface(EGL_DISPLAY(), *aConfig,
                                                       (EGLNativePixmapType)xsurface->XDrawable(),
                                                       pixmap_config_rgb);
        else
            surface = sEGLLibrary.fCreatePixmapSurface(EGL_DISPLAY(), *aConfig,
                                                       (EGLNativePixmapType)xsurface->XDrawable(),
                                                       pixmap_config_rgba);

        if (surface != EGL_NO_SURFACE)
            return surface;
    }

    EGLConfig configs[32];
    int numConfigs = 32;

    static EGLint pixmap_config[] = {
        LOCAL_EGL_SURFACE_TYPE,         LOCAL_EGL_PIXMAP_BIT,
        LOCAL_EGL_RENDERABLE_TYPE,      LOCAL_EGL_OPENGL_ES2_BIT,
        LOCAL_EGL_DEPTH_SIZE,           0,
        LOCAL_EGL_BIND_TO_TEXTURE_RGB,  LOCAL_EGL_TRUE,
        LOCAL_EGL_NONE
    };

    static EGLint pixmap_lock_config[] = {
        LOCAL_EGL_SURFACE_TYPE,         LOCAL_EGL_PIXMAP_BIT | LOCAL_EGL_LOCK_SURFACE_BIT_KHR,
        LOCAL_EGL_RENDERABLE_TYPE,      LOCAL_EGL_OPENGL_ES2_BIT,
        LOCAL_EGL_DEPTH_SIZE,           0,
        LOCAL_EGL_BIND_TO_TEXTURE_RGB,  LOCAL_EGL_TRUE,
        LOCAL_EGL_NONE
    };

    if (!sEGLLibrary.fChooseConfig(EGL_DISPLAY(),
                                   sEGLLibrary.HasKHRLockSurface() ?
                                       pixmap_lock_config : pixmap_config,
                                   configs, numConfigs, &numConfigs)
        || numConfigs == 0)
    {
        NS_WARNING("No EGL Config for pixmap!");
        return nullptr;
    }

    int i = 0;
    for (i = 0; i < numConfigs; ++i) {
        if (opaque)
            surface = sEGLLibrary.fCreatePixmapSurface(EGL_DISPLAY(), configs[i],
                                                       (EGLNativePixmapType)xsurface->XDrawable(),
                                                       pixmap_config_rgb);
        else
            surface = sEGLLibrary.fCreatePixmapSurface(EGL_DISPLAY(), configs[i],
                                                       (EGLNativePixmapType)xsurface->XDrawable(),
                                                       pixmap_config_rgba);

        if (surface != EGL_NO_SURFACE)
            break;
    }

    if (!surface) {
        NS_WARNING("Failed to CreatePixmapSurface!");
        return nullptr;
    }

    if (aConfig)
        *aConfig = configs[i];

    return surface;
}
#endif

already_AddRefed<GLContextEGL>
GLContextEGL::CreateEGLPixmapOffscreenContext(const gfxIntSize& size)
{
    gfxASurface *thebesSurface = nullptr;
    EGLNativePixmapType pixmap = 0;

#ifdef MOZ_X11
    nsRefPtr<gfxXlibSurface> xsurface =
        gfxXlibSurface::Create(DefaultScreenOfDisplay(DefaultXDisplay()),
                               gfxXlibSurface::FindRenderFormat(DefaultXDisplay(),
                                                                gfxASurface::ImageFormatRGB24),
                               size);

    
    XSync(DefaultXDisplay(), False);
    if (xsurface->CairoStatus() != 0)
        return nullptr;

    thebesSurface = xsurface;
    pixmap = (EGLNativePixmapType)xsurface->XDrawable();
#endif

    if (!pixmap) {
        return nullptr;
    }

    EGLSurface surface = 0;
    EGLConfig config = 0;

#ifdef MOZ_X11
    surface = CreateEGLSurfaceForXSurface(thebesSurface, &config);
#endif
    if (!config) {
        return nullptr;
    }
    MOZ_ASSERT(surface);

    GLContextEGL* shareContext = GetGlobalContextEGL();
    SurfaceCaps dummyCaps = SurfaceCaps::Any();
    nsRefPtr<GLContextEGL> glContext =
        GLContextEGL::CreateGLContext(dummyCaps,
                                      shareContext, true,
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
GLContextProviderEGL::CreateOffscreen(const gfxIntSize& size,
                                      const SurfaceCaps& caps,
                                      ContextFlags flags)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nullptr;
    }

    gfxIntSize dummySize = gfxIntSize(16, 16);
    nsRefPtr<GLContextEGL> glContext;
#if defined(MOZ_X11)
    glContext = GLContextEGL::CreateEGLPixmapOffscreenContext(dummySize);
#else
    glContext = GLContextEGL::CreateEGLPBufferOffscreenContext(dummySize);
#endif

    if (!glContext)
        return nullptr;

    if (flags & GLContext::ContextFlagsGlobal)
        return glContext.forget();

    if (!glContext->InitOffscreen(size, caps))
        return nullptr;

    return glContext.forget();
}




GLContext *
GLContextProviderEGL::GetGlobalContext(const ContextFlags)
{
    return nullptr;
}

void
GLContextProviderEGL::Shutdown()
{
    gGlobalContext = nullptr;
}

} 
} 

