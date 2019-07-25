








































#if defined(MOZ_X11)

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkx.h>

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)GDK_WINDOW_XID((GdkWindow *) aWidget->GetNativeData(NS_NATIVE_WINDOW))
#elif defined(MOZ_WIDGET_QT)
#include <QWidget>
#include <QtOpenGL/QGLWidget>
#define GLdouble_defined 1

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)static_cast<QWidget*>(aWidget->GetNativeData(NS_NATIVE_SHELLWIDGET))->handle()
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gfxXlibSurface.h"
typedef Display *EGLNativeDisplayType;
typedef Pixmap   EGLNativePixmapType;
typedef Window   EGLNativeWindowType;

#define EGL_LIB "/usr/lib/libEGL.so"
#define GLES2_LIB "/usr/lib/libGLESv2.so"

#elif defined(ANDROID)

#include <android/log.h>
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gecko" , ## args)


#include "AndroidBridge.h"

typedef void *EGLNativeDisplayType;
typedef void *EGLNativePixmapType;
typedef void *EGLNativeWindowType;

#define EGL_LIB "/system/lib/libEGL.so"
#define GLES2_LIB "/system/lib/libGLESv2.so"

#else

#error "Platform not recognized"

#endif

#include "gfxASurface.h"
#include "gfxPlatform.h"
#include "GLContextProvider.h"
#include "nsDebug.h"

#include "nsIWidget.h"

namespace mozilla {
namespace gl {

typedef int EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef void *EGLCastToRelevantPtr;
typedef void *EGLImageKHR;
typedef void *GLeglImageOES;

#define EGL_DEFAULT_DISPLAY  ((EGLNativeDisplayType)0)
#define EGL_NO_CONTEXT       ((EGLContext)0)
#define EGL_NO_DISPLAY       ((EGLDisplay)0)
#define EGL_NO_SURFACE       ((EGLSurface)0)

GLContextProvider sGLContextProvider;


static class EGLLibrary
{
public:
    EGLLibrary() : mInitialized(PR_FALSE) {}

    typedef EGLDisplay (*pfnGetDisplay)(void *display_id);
    pfnGetDisplay fGetDisplay;
    typedef EGLContext (*pfnGetCurrentContext)(void);
    pfnGetCurrentContext fGetCurrentContext;
    typedef EGLBoolean (*pfnMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
    pfnMakeCurrent fMakeCurrent;
    typedef EGLBoolean (*pfnDestroyContext)(EGLDisplay dpy, EGLContext ctx);
    pfnDestroyContext fDestroyContext;
    typedef EGLContext (*pfnCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
    pfnCreateContext fCreateContext;
    typedef EGLBoolean (*pfnDestroySurface)(EGLDisplay dpy, EGLSurface surface);
    pfnDestroySurface fDestroySurface;
    typedef EGLSurface (*pfnCreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
    pfnCreateWindowSurface fCreateWindowSurface;
    typedef EGLSurface (*pfnCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
    pfnCreatePbufferSurface fCreatePbufferSurface;
    typedef EGLSurface (*pfnCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
    pfnCreatePixmapSurface fCreatePixmapSurface;
    typedef EGLBoolean (*pfnBindAPI)(EGLenum api);
    pfnBindAPI fBindAPI;
    typedef EGLBoolean (*pfnInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
    pfnInitialize fInitialize;
    typedef EGLBoolean (*pfnChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
    pfnChooseConfig fChooseConfig;
    typedef EGLint (*pfnGetError)(void);
    pfnGetError fGetError;
    typedef EGLBoolean (*pfnGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
    pfnGetConfigAttrib fGetConfigAttrib;
    typedef EGLBoolean (*pfnGetConfigs)(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
    pfnGetConfigs fGetConfigs;
    typedef EGLBoolean (*pfnWaitNative)(EGLint engine);
    pfnWaitNative fWaitNative;
    typedef EGLCastToRelevantPtr (*pfnGetProcAddress)(const char *procname);
    pfnGetProcAddress fGetProcAddress;
    typedef EGLBoolean (*pfnSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
    pfnSwapBuffers fSwapBuffers;
    typedef EGLBoolean (*pfnCopyBuffers)(EGLDisplay dpy, EGLSurface surface,
                                         EGLNativePixmapType target);
    pfnCopyBuffers fCopyBuffers;
    typedef const GLubyte* (*pfnQueryString)(EGLDisplay, EGLint name);
    pfnQueryString fQueryString;
    typedef EGLBoolean (*pfnBindTexImage)(EGLDisplay, EGLSurface surface, EGLint buffer);
    pfnBindTexImage fBindTexImage;
    typedef EGLBoolean (*pfnReleaseTexImage)(EGLDisplay, EGLSurface surface, EGLint buffer);
    pfnReleaseTexImage fReleaseTexImage;
    typedef EGLImageKHR (*pfnCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
    pfnCreateImageKHR fCreateImageKHR;
    typedef EGLBoolean (*pfnDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
    pfnDestroyImageKHR fDestroyImageKHR;
    
    
    typedef void (*pfnImageTargetTexture2DOES)(GLenum target, GLeglImageOES image);
    pfnImageTargetTexture2DOES fImageTargetTexture2DOES;

    PRBool EnsureInitialized()
    {
        if (mInitialized) {
            return PR_TRUE;
        }

        if (!mEGLLibrary) {
            mEGLLibrary = PR_LoadLibrary(EGL_LIB);
            if (!mEGLLibrary) {
                NS_WARNING("Couldn't load EGL LIB.");
                return PR_FALSE;
            }
        }
#define SYMBOL(name) \
    { (PRFuncPtr*) &f##name, { "egl" #name, NULL } }

        LibrarySymbolLoader::SymLoadStruct earlySymbols[] = {
            SYMBOL(GetDisplay),
            SYMBOL(GetCurrentContext),
            SYMBOL(MakeCurrent),
            SYMBOL(DestroyContext),
            SYMBOL(CreateContext),
            SYMBOL(DestroySurface),
            SYMBOL(CreateWindowSurface),
            SYMBOL(CreatePbufferSurface),
            SYMBOL(CreatePixmapSurface),
            SYMBOL(BindAPI),
            SYMBOL(Initialize),
            SYMBOL(ChooseConfig),
            SYMBOL(GetError),
            SYMBOL(GetConfigs),
            SYMBOL(GetConfigAttrib),
            SYMBOL(WaitNative),
            SYMBOL(GetProcAddress),
            SYMBOL(SwapBuffers),
            SYMBOL(CopyBuffers),
            SYMBOL(QueryString),
            SYMBOL(BindTexImage),
            SYMBOL(ReleaseTexImage),
            { NULL, { NULL } }
        };

        if (!LibrarySymbolLoader::LoadSymbols(mEGLLibrary, &earlySymbols[0])) {
            NS_WARNING("Couldn't find required entry points in EGL library (early init)");
            return PR_FALSE;
        }

        LibrarySymbolLoader::SymLoadStruct khrSymbols[] = {
            { (PRFuncPtr*) &fCreateImageKHR, { "eglCreateImageKHR", NULL } },
            { (PRFuncPtr*) &fDestroyImageKHR, { "eglDestroyImageKHR", NULL } },
            { (PRFuncPtr*) &fImageTargetTexture2DOES, { "glEGLImageTargetTexture2DOES", NULL } },
            { NULL, { NULL } }
        };

        if (!LibrarySymbolLoader::LoadSymbols(mEGLLibrary, &khrSymbols[0],
               (LibrarySymbolLoader::PlatformLookupFunction)fGetProcAddress))
        {
            
            fCreateImageKHR = nsnull;
        }

        mInitialized = PR_TRUE;
        return PR_TRUE;
    }

private:
    PRBool mInitialized;
    PRLibrary *mEGLLibrary;
} sEGLLibrary;

class GLContextEGL : public GLContext
{
    friend class TextureImageEGL;

public:
    GLContextEGL(EGLDisplay aDisplay, EGLConfig aConfig,
                 EGLSurface aSurface, EGLContext aContext,
                 void *aGLWidget = nsnull,
                 gfxASurface *aASurface = nsnull)
        : mDisplay(aDisplay), mConfig(aConfig) 
        , mSurface(aSurface), mContext(aContext)
        , mGLWidget(aGLWidget)
        , mASurface(aASurface)
        , mBound(PR_FALSE)
    {}

    ~GLContextEGL()
    {
        
        
        
        if (mGLWidget)
            return;

        sEGLLibrary.fDestroyContext(mDisplay, mContext);
        sEGLLibrary.fDestroySurface(mDisplay, mSurface);
    }

    PRBool Init()
    {
        if (!OpenLibrary(GLES2_LIB)) {
            NS_WARNING("Couldn't load EGL LIB.");
            return PR_FALSE;
        }

        MakeCurrent();
        return InitWithPrefix("gl", PR_TRUE);
    }

    PRBool BindTexImage()
    {
        if (!mSurface)
            return PR_FALSE;

        if (mBound && !ReleaseTexImage())
            return PR_FALSE;

        EGLBoolean success = sEGLLibrary.fBindTexImage(mDisplay,
            (EGLSurface)mSurface, LOCAL_EGL_BACK_BUFFER);
        if (success == LOCAL_EGL_FALSE)
            return PR_FALSE;

        mBound = PR_TRUE;
        return PR_TRUE;
    }

    PRBool ReleaseTexImage()
    {
        if (!mBound)
            return PR_TRUE;

        if (!mDisplay || !mSurface)
            return PR_FALSE;

        EGLBoolean success;
        success = sEGLLibrary.fReleaseTexImage(mDisplay, (EGLSurface)mSurface, LOCAL_EGL_BACK_BUFFER);
        if (success == LOCAL_EGL_FALSE)
            return PR_FALSE;

        mBound = PR_FALSE;
        return PR_TRUE;
    }

    PRBool MakeCurrent()
    {
        PRBool succeeded = PR_TRUE;

        
        
        
        if (sEGLLibrary.fGetCurrentContext() != mContext) {
            if (mGLWidget) {
#ifdef MOZ_WIDGET_QT
                static_cast<QGLWidget*>(mGLWidget)->makeCurrent();
#else
                succeeded = PR_FALSE;
#endif
            } else {
                succeeded = sEGLLibrary.fMakeCurrent(mDisplay, mSurface, mSurface, mContext);
            }
            NS_ASSERTION(succeeded, "Failed to make GL context current!");
        }

        return succeeded;
    }

    PRBool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sEGLLibrary.fGetProcAddress;
        return PR_TRUE;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch (aType) {
        case NativeGLContext:
            return mContext;

        case NativePBuffer:
            return mSurface;

        default:
            return nsnull;
        }
    }

    PRBool SwapBuffers()
    {
        return sEGLLibrary.fSwapBuffers(mDisplay, mSurface);
    }

    virtual already_AddRefed<TextureImage>
    CreateTextureImage(const nsIntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLint aWrapMode,
                       PRBool aUseNearestFilter=PR_FALSE);

private:
    EGLDisplay mDisplay;
    EGLConfig  mConfig;
    EGLSurface mSurface;
    EGLContext mContext;
    void      *mGLWidget;
    nsRefPtr <gfxASurface> mASurface;
    PRBool     mBound;
};

class TextureImageEGL : public TextureImage
{
public:
    TextureImageEGL(GLuint aTexture,
                    const nsIntSize& aSize,
                    ContentType aContentType,
                    GLContext* aContext,
                    GLContextEGL* aImpl)
        : TextureImage(aTexture, aSize, aContentType)
        , mGLContext(aContext)
        , mImpl(aImpl)
    { }

    virtual ~TextureImageEGL()
    {
        mGLContext->MakeCurrent();
        mImpl->ReleaseTexImage();
        mGLContext->fDeleteTextures(1, &mTexture);
        mImpl = NULL;
    }

    virtual gfxContext* BeginUpdate(nsIntRegion& aRegion)
    {
        NS_ASSERTION(!mUpdateContext, "BeginUpdate() without EndUpdate()?");

        mUpdateContext = new gfxContext(mImpl->mASurface);
        
        
        return mUpdateContext;
    }

    virtual PRBool EndUpdate()
    {
        NS_ASSERTION(mUpdateContext, "EndUpdate() without BeginUpdate()?");

#ifdef MOZ_X11
        
        
#endif  

        
        
        mUpdateContext = NULL;
        return PR_FALSE;        
    }

private:
    GLContext* mGLContext;
    nsRefPtr<GLContextEGL> mImpl;
    nsRefPtr<gfxContext> mUpdateContext;
};

already_AddRefed<TextureImage>
GLContextEGL::CreateTextureImage(const nsIntSize& aSize,
                                 TextureImage::ContentType aContentType,
                                 GLint aWrapMode,
                                 PRBool aUseNearestFilter)
{
  gfxASurface::gfxImageFormat imageFormat =
      (gfxASurface::CONTENT_COLOR == aContentType) ?
      gfxASurface::ImageFormatRGB24 : gfxASurface::ImageFormatARGB32;

  nsRefPtr<gfxASurface> pixmap =
    gfxPlatform::GetPlatform()->
      CreateOffscreenSurface(gfxIntSize(aSize.width, aSize.height),
                             imageFormat);

  nsRefPtr<GLContext> impl =
      sGLContextProvider.CreateForNativePixmapSurface(pixmap);
  if (!impl)
      
      return NULL;

  MakeCurrent();

  GLuint texture;
  fGenTextures(1, &texture);

  fActiveTexture(LOCAL_GL_TEXTURE0);
  fBindTexture(LOCAL_GL_TEXTURE_2D, texture);

  GLint texfilter = aUseNearestFilter ? LOCAL_GL_NEAREST : LOCAL_GL_LINEAR;
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, texfilter);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, texfilter);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, aWrapMode);
  fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, aWrapMode);

  impl->BindTexImage();

  nsRefPtr<TextureImageEGL> teximage =
      new TextureImageEGL(texture, aSize, aContentType, this,
                          static_cast<GLContextEGL*>(impl.get()));
  return teximage.forget();
}

already_AddRefed<GLContext>
GLContextProvider::CreateForWindow(nsIWidget *aWidget)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

#ifdef MOZ_WIDGET_QT
    QWidget *viewport = static_cast<QWidget*>(aWidget->GetNativeData(NS_NATIVE_SHELLWIDGET));
    if (!viewport)
        return nsnull;

    if (viewport->paintEngine()->type() == QPaintEngine::OpenGL2) {
        
        
        nsRefPtr<GLContextEGL> glContext =
            new GLContextEGL(NULL, NULL, NULL,
                             sEGLLibrary.fGetCurrentContext(),
                             viewport);
        if (!glContext->Init())
            return nsnull;
        return glContext.forget().get();
    } else {
        
        
        
        NS_WARNING("Need special GLContext implementation for QT widgets structure");
        
        return nsnull;
    }
#endif

    EGLDisplay display;
    EGLConfig  config;
    EGLSurface surface;
    EGLContext context;

    display = sEGLLibrary.fGetDisplay(aWidget->GetNativeData(NS_NATIVE_DISPLAY));
    if (!display) {
        return nsnull;
    }

    if (!sEGLLibrary.fInitialize(display, NULL, NULL)) {
        return nsnull;
    }

    EGLint attribs[] = {
        LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
        LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
        LOCAL_EGL_RED_SIZE,        5,
        LOCAL_EGL_GREEN_SIZE,      6,
        LOCAL_EGL_BLUE_SIZE,       5,
        LOCAL_EGL_ALPHA_SIZE,      0,
#endif

        LOCAL_EGL_NONE
    };

    EGLint ncfg = 0;
    if (!sEGLLibrary.fChooseConfig(display, attribs, &config, 1, &ncfg) ||
        ncfg < 1)
    {
        return nsnull;
    }

#ifdef ANDROID
    
    
    
    
    
    surface = mozilla::AndroidBridge::Bridge()->
        CallEglCreateWindowSurface(display, config,
                                   mozilla::AndroidBridge::Bridge()->SurfaceView());
#else
    surface = sEGLLibrary.fCreateWindowSurface(display, config, GET_NATIVE_WINDOW(aWidget), 0);
#endif

    if (!surface) {
        return nsnull;
    }

    if (!sEGLLibrary.fBindAPI(LOCAL_EGL_OPENGL_ES_API)) {
        sEGLLibrary.fDestroySurface(display, surface);
        return nsnull;
    }

    EGLint cxattribs[] = {
        LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
        LOCAL_EGL_NONE
    };

    context = sEGLLibrary.fCreateContext(display, config, 0, cxattribs);
    if (!context) {
        sEGLLibrary.fDestroySurface(display, surface);
        return nsnull;
    }

    nsRefPtr<GLContextEGL> glContext = new GLContextEGL(display, config, surface, context);

    if (!glContext->Init())
        return nsnull;

    return glContext.forget().get();
}

already_AddRefed<GLContext>
GLContextProvider::CreatePBuffer(const gfxIntSize &aSize, const ContextFormat &aFormat)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    display = sEGLLibrary.fGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!sEGLLibrary.fInitialize(display, NULL, NULL))
        return nsnull;

    nsTArray<int> attribs;

#define A1_(_x)  do { attribs.AppendElement(_x); } while(0)
#define A2_(_x,_y)  do {                                                \
        attribs.AppendElement(_x);                                      \
        attribs.AppendElement(_y);                                      \
    } while(0)

    A2_(LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT);
    A2_(LOCAL_EGL_SURFACE_TYPE, LOCAL_EGL_PBUFFER_BIT);
    
    A2_(LOCAL_EGL_BUFFER_SIZE, 15 );
    A2_(LOCAL_EGL_RED_SIZE, 4 );
    A2_(LOCAL_EGL_GREEN_SIZE, 4 );
    A2_(LOCAL_EGL_BLUE_SIZE, 4 );
    A2_(LOCAL_EGL_ALPHA_SIZE, aFormat.alpha ? 4 : 0);
    A2_(LOCAL_EGL_DEPTH_SIZE, aFormat.depth ? 16 : 0);
    A2_(LOCAL_EGL_STENCIL_SIZE, aFormat.stencil);
    A1_(LOCAL_EGL_NONE);

    EGLConfig configs[32];
    int numConfigs = 32;

    if (!sEGLLibrary.fChooseConfig(display, attribs.Elements(),
                                   configs, numConfigs,
                                   &numConfigs))
        return nsnull;

    if (numConfigs == 0)
        return nsnull;

    
    config = configs[0];

    EGLint pbattrs[] = {
        LOCAL_EGL_WIDTH, aSize.width,
        LOCAL_EGL_HEIGHT, aSize.height,
        LOCAL_EGL_NONE
    };

    surface = sEGLLibrary.fCreatePbufferSurface(display, config, pbattrs);
    if (!surface)
        return nsnull;

    sEGLLibrary.fBindAPI(LOCAL_EGL_OPENGL_ES_API);

    EGLint cxattrs[] = {
        LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
        LOCAL_EGL_NONE
    };

    context = sEGLLibrary.fCreateContext(display, config, EGL_NO_CONTEXT, cxattrs);
    if (!context) {
        sEGLLibrary.fDestroySurface(display, surface);
        return nsnull;
    }

    nsRefPtr<GLContextEGL> glContext = new GLContextEGL(display, config, surface, context);

    if (!glContext->Init())
        return nsnull;

    return glContext.forget().get();
}

already_AddRefed<GLContext>
GLContextProvider::CreateForNativePixmapSurface(gfxASurface *aSurface)
{
    EGLDisplay display = nsnull;
    EGLSurface surface = nsnull;
    EGLContext context = nsnull;

    if (!sEGLLibrary.EnsureInitialized())
        return nsnull;

#ifdef MOZ_X11
    if (aSurface->GetType() != gfxASurface::SurfaceTypeXlib) {
        
        return nsnull;
    }

    gfxXlibSurface *xsurface = static_cast<gfxXlibSurface*>(aSurface);

    display = sEGLLibrary.fGetDisplay((EGLNativeDisplayType)xsurface->XDisplay());
    if (!display)
        return nsnull;

    if (!sEGLLibrary.fInitialize(display, NULL, NULL))
        return nsnull;

    EGLConfig configs[32];
    int numConfigs = 32;

    EGLint pixmap_config[] = {
        LOCAL_EGL_SURFACE_TYPE,         LOCAL_EGL_PIXMAP_BIT,
        LOCAL_EGL_RENDERABLE_TYPE,      LOCAL_EGL_OPENGL_ES2_BIT,
        LOCAL_EGL_DEPTH_SIZE,           0,
        LOCAL_EGL_BIND_TO_TEXTURE_RGB,  LOCAL_EGL_TRUE,
        LOCAL_EGL_NONE
    };

    EGLint pixmap_config_rgb[] = {
        LOCAL_EGL_TEXTURE_TARGET,       LOCAL_EGL_TEXTURE_2D,
        LOCAL_EGL_TEXTURE_FORMAT,       LOCAL_EGL_TEXTURE_RGB,
        LOCAL_EGL_NONE
    };

    EGLint pixmap_config_rgba[] = {
        LOCAL_EGL_TEXTURE_TARGET,       LOCAL_EGL_TEXTURE_2D,
        LOCAL_EGL_TEXTURE_FORMAT,       LOCAL_EGL_TEXTURE_RGBA,
        LOCAL_EGL_NONE
    };

    if (!sEGLLibrary.fChooseConfig(display, pixmap_config,
                                   configs, numConfigs, &numConfigs))
        return nsnull;

    if (numConfigs == 0)
        return nsnull;

    PRBool opaque =
        aSurface->GetContentType() == gfxASurface::CONTENT_COLOR;
    int i = 0;
    for (i = 0; i < numConfigs; ++i) {
        if (opaque)
            surface = sEGLLibrary.fCreatePixmapSurface(display, configs[i],
                                                       xsurface->XDrawable(),
                                                       pixmap_config_rgb);
        else
            surface = sEGLLibrary.fCreatePixmapSurface(display, configs[i],
                                                       xsurface->XDrawable(),
                                                       pixmap_config_rgba);

        if (surface != EGL_NO_SURFACE)
            break;
    }


    EGLint cxattribs[] = {
        LOCAL_EGL_CONTEXT_CLIENT_VERSION, 2,
        LOCAL_EGL_NONE
    };

    context = sEGLLibrary.fCreateContext(display, configs[i], 0, cxattribs);
    if (!context) {
        sEGLLibrary.fDestroySurface(display, surface);
        return nsnull;
    }

    nsRefPtr<GLContextEGL> glContext =
        new GLContextEGL(display, configs[i], surface, context, NULL, aSurface);

    return glContext.forget().get();
#else
    
    return nsnull;
#endif
}

} 
} 

