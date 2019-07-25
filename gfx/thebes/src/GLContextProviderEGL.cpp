








































#if defined(MOZ_X11)

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkx.h>

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)GDK_WINDOW_XID((GdkWindow *) aWidget->GetNativeData(NS_NATIVE_WINDOW))
#elif defined(MOZ_WIDGET_QT)
#include <QWidget>

#define GET_NATIVE_WINDOW(aWidget) (EGLNativeWindowType)static_cast<QWidget*>(aWidget->GetNativeData(NS_NATIVE_SHELLWIDGET))->handle()
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
typedef Display *EGLNativeDisplayType;
typedef Pixmap   EGLNativePixmapType;
typedef Window   EGLNativeWindowType;

#define EGL_LIB "/usr/lib/libEGL.so"
#define GLES2_LIB "/usr/lib/libGLESv2.so"

#elif defined(ANDROID)

#define GET_NATIVE_WINDOW(aWidget)  (nsnull)

typedef void *EGLNativeDisplayType;
typedef void *EGLNativePixmapType;
typedef void *EGLNativeWindowType;

#define EGL_LIB "/system/lib/libEGL.so"
#define GLES2_LIB "/system/lib/libGLESv2.so"

#else

#error "Platform not recognized"

#endif

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
	    { NULL, { NULL } }
	};

	if (!LibrarySymbolLoader::LoadSymbols(mEGLLibrary, &earlySymbols[0])) {
	    NS_WARNING("Couldn't find required entry points in EGL library (early init)");
	    return PR_FALSE;
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
public:
    GLContextEGL(EGLDisplay aDisplay, EGLConfig aConfig, 
                 EGLSurface aSurface, EGLContext aContext)
        : mDisplay(aDisplay), mConfig(aConfig), 
          mSurface(aSurface), mContext(aContext){}

    ~GLContextEGL()
    {
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

    PRBool MakeCurrent()
    {
	PRBool succeeded = PR_TRUE;

	
	
	
	if (sEGLLibrary.fGetCurrentContext() != mContext) {
	    succeeded = sEGLLibrary.fMakeCurrent(mDisplay, mSurface, mSurface, mContext);
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

private:
    EGLDisplay mDisplay;
    EGLConfig  mConfig;
    EGLSurface mSurface;
    EGLContext mContext;
};

already_AddRefed<GLContext>
GLContextProvider::CreateForWindow(nsIWidget *aWidget)
{
    if (!sEGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

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

    if (!sEGLLibrary.fBindAPI(LOCAL_EGL_OPENGL_ES_API)) {
        return nsnull;
    }

    EGLint attribs[] = {
        LOCAL_EGL_SURFACE_TYPE,    LOCAL_EGL_WINDOW_BIT,
        LOCAL_EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
        LOCAL_EGL_NONE
    };

    EGLint ncfg = 0;
    if (!sEGLLibrary.fChooseConfig(display, attribs, &config, 1, &ncfg) ||
        ncfg < 1)
    {
        return nsnull;
    }

    surface = sEGLLibrary.fCreateWindowSurface(display, config, GET_NATIVE_WINDOW(aWidget), 0);
    if (!surface) {
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

} 
} 

