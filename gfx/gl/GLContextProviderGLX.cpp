




#ifdef MOZ_WIDGET_GTK
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#define GET_NATIVE_WINDOW(aWidget) GDK_WINDOW_XID((GdkWindow *) aWidget->GetNativeData(NS_NATIVE_WINDOW))
#elif defined(MOZ_WIDGET_QT)
#include <QWidget>
#define GET_NATIVE_WINDOW(aWidget) static_cast<QWidget*>(aWidget->GetNativeData(NS_NATIVE_SHELLWIDGET))->winId()
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "mozilla/X11Util.h"

#include "prenv.h"
#include "prbit.h" 
#include "GLContextProvider.h"
#include "GLLibraryLoader.h"
#include "nsDebug.h"
#include "nsIWidget.h"
#include "GLXLibrary.h"
#include "gfxXlibSurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "GLContext.h"
#include "gfxUtils.h"

#include "gfxCrashReporterUtils.h"

#ifdef MOZ_WIDGET_GTK
#include "gfxPlatformGtk.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

GLXLibrary sGLXLibrary[GLXLibrary::LIBS_MAX];
GLXLibrary& sDefGLXLib = sGLXLibrary[GLXLibrary::OPENGL_LIB];

typedef GLXLibrary::LibraryType LibType;

static LibType gCurrLib = GLXLibrary::OPENGL_LIB;

LibType
GLXLibrary::SelectLibrary(const ContextFlags& aFlags)
{
  return (aFlags & ContextFlagsMesaLLVMPipe)
          ? GLXLibrary::MESA_LLVMPIPE_LIB
          : GLXLibrary::OPENGL_LIB;
}


bool 
GLXLibrary::GLXVersionCheck(int aMajor, int aMinor)
{
    return aMajor < mGLXMajorVersion ||
           (aMajor == mGLXMajorVersion && aMinor <= mGLXMinorVersion);
}

static inline bool
HasExtension(const char* aExtensions, const char* aRequiredExtension)
{
    return GLContext::ListHasExtension(
        reinterpret_cast<const GLubyte*>(aExtensions), aRequiredExtension);
}

bool
GLXLibrary::EnsureInitialized(LibType libType)
{
    if (mInitialized) {
        return true;
    }

    
    if (mTriedInitializing) {
        return false;
    }
    mTriedInitializing = true;

    
    PR_SetEnv("force_s3tc_enable=true");

    if (!mOGLLibrary) {
        const char* libGLfilename = nullptr;
        bool forceFeatureReport = false;
        switch (libType) {
        case MESA_LLVMPIPE_LIB:
            libGLfilename = "mesallvmpipe.so";
            forceFeatureReport = true;
            break;
        case OPENGL_LIB:
            
            
            
#ifdef __OpenBSD__
            libGLfilename = "libGL.so";
#else
            libGLfilename = "libGL.so.1";
#endif
            break;
        default:
            MOZ_CRASH("Invalid GLX library type.");
        }

        ScopedGfxFeatureReporter reporter(libGLfilename, forceFeatureReport);
        mOGLLibrary = PR_LoadLibrary(libGLfilename);
        if (!mOGLLibrary) {
            NS_WARNING("Couldn't load OpenGL shared library.");
            return false;
        }
        reporter.SetSuccessful();
    }

    if (PR_GetEnv("MOZ_GLX_DEBUG")) {
        mDebug = true;
    }

    GLLibraryLoader::SymLoadStruct symbols[] = {
        
        { (PRFuncPtr*) &xDestroyContextInternal, { "glXDestroyContext", nullptr } },
        { (PRFuncPtr*) &xMakeCurrentInternal, { "glXMakeCurrent", nullptr } },
        { (PRFuncPtr*) &xSwapBuffersInternal, { "glXSwapBuffers", nullptr } },
        { (PRFuncPtr*) &xQueryVersionInternal, { "glXQueryVersion", nullptr } },
        { (PRFuncPtr*) &xGetCurrentContextInternal, { "glXGetCurrentContext", nullptr } },
        { (PRFuncPtr*) &xWaitGLInternal, { "glXWaitGL", nullptr } },
        { (PRFuncPtr*) &xWaitXInternal, { "glXWaitX", nullptr } },
        
        { (PRFuncPtr*) &xQueryExtensionsStringInternal, { "glXQueryExtensionsString", nullptr } },
        { (PRFuncPtr*) &xGetClientStringInternal, { "glXGetClientString", nullptr } },
        { (PRFuncPtr*) &xQueryServerStringInternal, { "glXQueryServerString", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols13[] = {
        
        { (PRFuncPtr*) &xChooseFBConfigInternal, { "glXChooseFBConfig", nullptr } },
        { (PRFuncPtr*) &xGetFBConfigAttribInternal, { "glXGetFBConfigAttrib", nullptr } },
        
        { (PRFuncPtr*) &xGetFBConfigsInternal, { "glXGetFBConfigs", nullptr } },
        
        { (PRFuncPtr*) &xCreatePixmapInternal, { "glXCreatePixmap", nullptr } },
        { (PRFuncPtr*) &xDestroyPixmapInternal, { "glXDestroyPixmap", nullptr } },
        { (PRFuncPtr*) &xCreateNewContextInternal, { "glXCreateNewContext", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols13_ext[] = {
        
        
        { (PRFuncPtr*) &xChooseFBConfigInternal, { "glXChooseFBConfigSGIX", nullptr } },
        { (PRFuncPtr*) &xGetFBConfigAttribInternal, { "glXGetFBConfigAttribSGIX", nullptr } },
        
        
        { (PRFuncPtr*) &xCreateGLXPixmapWithConfigInternal, { "glXCreateGLXPixmapWithConfigSGIX", nullptr } },
        { (PRFuncPtr*) &xDestroyPixmapInternal, { "glXDestroyGLXPixmap", nullptr } }, 
        { (PRFuncPtr*) &xCreateNewContextInternal, { "glXCreateContextWithConfigSGIX", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols14[] = {
        
        { (PRFuncPtr*) &xGetProcAddressInternal, { "glXGetProcAddress", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols14_ext[] = {
        
        
        { (PRFuncPtr*) &xGetProcAddressInternal, { "glXGetProcAddressARB", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols_texturefrompixmap[] = {
        { (PRFuncPtr*) &xBindTexImageInternal, { "glXBindTexImageEXT", nullptr } },
        { (PRFuncPtr*) &xReleaseTexImageInternal, { "glXReleaseTexImageEXT", nullptr } },
        { nullptr, { nullptr } }
    };

    GLLibraryLoader::SymLoadStruct symbols_robustness[] = {
        { (PRFuncPtr*) &xCreateContextAttribsInternal, { "glXCreateContextAttribsARB", nullptr } },
        { nullptr, { nullptr } }
    };

    if (!GLLibraryLoader::LoadSymbols(mOGLLibrary, &symbols[0])) {
        NS_WARNING("Couldn't find required entry point in OpenGL shared library");
        return false;
    }

    Display *display = DefaultXDisplay();
    int screen = DefaultScreen(display);

    if (!xQueryVersion(display, &mGLXMajorVersion, &mGLXMinorVersion)) {
        mGLXMajorVersion = 0;
        mGLXMinorVersion = 0;
        return false;
    }

    if (!GLXVersionCheck(1, 1))
        
        return false;

    const char *clientVendor = xGetClientString(display, GLX_VENDOR);
    const char *serverVendor = xQueryServerString(display, screen, GLX_VENDOR);
    const char *extensionsStr = xQueryExtensionsString(display, screen);

    GLLibraryLoader::SymLoadStruct *sym13;
    if (!GLXVersionCheck(1, 3)) {
        
        
        if (!HasExtension(extensionsStr, "GLX_SGIX_fbconfig")) {
            return false;
        }
        sym13 = symbols13_ext;
    } else {
        sym13 = symbols13;
    }
    if (!GLLibraryLoader::LoadSymbols(mOGLLibrary, sym13)) {
        NS_WARNING("Couldn't find required entry point in OpenGL shared library");
        return false;
    }

    GLLibraryLoader::SymLoadStruct *sym14;
    if (!GLXVersionCheck(1, 4)) {
        
        
        if (!HasExtension(extensionsStr, "GLX_ARB_get_proc_address")) {
            return false;
        }
        sym14 = symbols14_ext;
    } else {
        sym14 = symbols14;
    }
    if (!GLLibraryLoader::LoadSymbols(mOGLLibrary, sym14)) {
        NS_WARNING("Couldn't find required entry point in OpenGL shared library");
        return false;
    }

    if (HasExtension(extensionsStr, "GLX_EXT_texture_from_pixmap") &&
        GLLibraryLoader::LoadSymbols(mOGLLibrary, symbols_texturefrompixmap, 
                                         (GLLibraryLoader::PlatformLookupFunction)&xGetProcAddress))
    {
#ifdef MOZ_WIDGET_GTK
        mUseTextureFromPixmap = gfxPlatformGtk::GetPlatform()->UseXRender();
#else
        mUseTextureFromPixmap = true;
#endif
    } else {
        mUseTextureFromPixmap = false;
        NS_WARNING("Texture from pixmap disabled");
    }

    if (HasExtension(extensionsStr, "GLX_ARB_create_context_robustness") &&
        GLLibraryLoader::LoadSymbols(mOGLLibrary, symbols_robustness)) {
        mHasRobustness = true;
    }

    mIsATI = serverVendor && DoesStringMatch(serverVendor, "ATI");
    mIsNVIDIA = serverVendor && DoesStringMatch(serverVendor, "NVIDIA Corporation");
    mClientIsMesa = clientVendor && DoesStringMatch(clientVendor, "Mesa");

    mInitialized = true;
    mLibType = libType;

    return true;
}

bool
GLXLibrary::SupportsTextureFromPixmap(gfxASurface* aSurface)
{
    if (!EnsureInitialized(mLibType)) {
        return false;
    }
    
    if (aSurface->GetType() != gfxSurfaceTypeXlib || !mUseTextureFromPixmap) {
        return false;
    }

    return true;
}

GLXPixmap 
GLXLibrary::CreatePixmap(gfxASurface* aSurface)
{
    if (!SupportsTextureFromPixmap(aSurface)) {
        return None;
    }

    gfxXlibSurface *xs = static_cast<gfxXlibSurface*>(aSurface);
    const XRenderPictFormat *format = xs->XRenderFormat();
    if (!format || format->type != PictTypeDirect) {
        return None;
    }
    const XRenderDirectFormat& direct = format->direct;
    int alphaSize;
    PR_FLOOR_LOG2(alphaSize, direct.alphaMask + 1);
    NS_ASSERTION((1 << alphaSize) - 1 == direct.alphaMask,
                 "Unexpected render format with non-adjacent alpha bits");

    int attribs[] = { GLX_DOUBLEBUFFER, False,
                      GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
                      GLX_ALPHA_SIZE, alphaSize,
                      (alphaSize ? GLX_BIND_TO_TEXTURE_RGBA_EXT
                       : GLX_BIND_TO_TEXTURE_RGB_EXT), True,
                      GLX_RENDER_TYPE, GLX_RGBA_BIT,
                      None };

    int numConfigs = 0;
    Display *display = xs->XDisplay();
    int xscreen = DefaultScreen(display);

    ScopedXFree<GLXFBConfig> cfgs(xChooseFBConfig(display,
                                                  xscreen,
                                                  attribs,
                                                  &numConfigs));

    
    int matchIndex = -1;
    unsigned long redMask =
        static_cast<unsigned long>(direct.redMask) << direct.red;
    unsigned long greenMask =
        static_cast<unsigned long>(direct.greenMask) << direct.green;
    unsigned long blueMask =
        static_cast<unsigned long>(direct.blueMask) << direct.blue;
    
    bool haveNonColorBits =
        ~(redMask | greenMask | blueMask) != -1UL << format->depth;

    for (int i = 0; i < numConfigs; i++) {
        int id = None;
        sGLXLibrary[mLibType].xGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &id);
        Visual *visual;
        int depth;
        FindVisualAndDepth(display, id, &visual, &depth);
        if (!visual ||
            visual->c_class != TrueColor ||
            visual->red_mask != redMask ||
            visual->green_mask != greenMask ||
            visual->blue_mask != blueMask ) {
            continue;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (depth != format->depth &&
            (mIsNVIDIA || depth != format->depth - alphaSize) ) {
            continue;
        }

        
        
        
        
        
        
        
        
        
        
        
        if (haveNonColorBits) {
            
            
            
            
            int size = 0;
            sGLXLibrary[mLibType].xGetFBConfigAttrib(display, cfgs[i],
                                                     GLX_ALPHA_SIZE, &size);
            if (size != alphaSize) {
                continue;
            }
        }

        matchIndex = i;
        break;
    }
    if (matchIndex == -1) {
        
        
        NS_WARN_IF_FALSE(format->depth == 8,
                         "[GLX] Couldn't find a FBConfig matching Pixmap format");
        return None;
    }

    int pixmapAttribs[] = { GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
                            GLX_TEXTURE_FORMAT_EXT,
                            (alphaSize ? GLX_TEXTURE_FORMAT_RGBA_EXT
                             : GLX_TEXTURE_FORMAT_RGB_EXT),
                            None};

    GLXPixmap glxpixmap = xCreatePixmap(display,
                                        cfgs[matchIndex],
                                        xs->XDrawable(),
                                        pixmapAttribs);

    return glxpixmap;
}

void
GLXLibrary::DestroyPixmap(GLXPixmap aPixmap)
{
    if (!mUseTextureFromPixmap) {
        return;
    }

    Display *display = DefaultXDisplay();
    xDestroyPixmap(display, aPixmap);
}

void
GLXLibrary::BindTexImage(GLXPixmap aPixmap)
{    
    if (!mUseTextureFromPixmap) {
        return;
    }

    Display *display = DefaultXDisplay();
    
    if (mClientIsMesa) {
        
        
        
        FinishX(display);
    } else {
        xWaitX();
    }
    xBindTexImage(display, aPixmap, GLX_FRONT_LEFT_EXT, nullptr);
}

void
GLXLibrary::ReleaseTexImage(GLXPixmap aPixmap)
{
    if (!mUseTextureFromPixmap) {
        return;
    }

    Display *display = DefaultXDisplay();
    xReleaseTexImage(display, aPixmap, GLX_FRONT_LEFT_EXT);
}

#ifdef DEBUG

static int (*sOldErrorHandler)(Display *, XErrorEvent *);
ScopedXErrorHandler::ErrorEvent sErrorEvent;
static int GLXErrorHandler(Display *display, XErrorEvent *ev)
{
    if (!sErrorEvent.mError.error_code) {
        sErrorEvent.mError = *ev;
    }
    return 0;
}

void
GLXLibrary::BeforeGLXCall()
{
    if (mDebug) {
        sOldErrorHandler = XSetErrorHandler(GLXErrorHandler);
    }
}

void
GLXLibrary::AfterGLXCall()
{
    if (mDebug) {
        FinishX(DefaultXDisplay());
        if (sErrorEvent.mError.error_code) {
            char buffer[2048];
            XGetErrorText(DefaultXDisplay(), sErrorEvent.mError.error_code, buffer, sizeof(buffer));
            printf_stderr("X ERROR: %s (%i) - Request: %i.%i, Serial: %i",
                          buffer,
                          sErrorEvent.mError.error_code,
                          sErrorEvent.mError.request_code,
                          sErrorEvent.mError.minor_code,
                          sErrorEvent.mError.serial);
            NS_ABORT();
        }
        XSetErrorHandler(sOldErrorHandler);
    }
}

#define BEFORE_GLX_CALL do {                     \
    sGLXLibrary[gCurrLib].BeforeGLXCall();       \
} while (0)
    
#define AFTER_GLX_CALL do {                      \
    sGLXLibrary[gCurrLib].AfterGLXCall();        \
} while (0)

#else

#define BEFORE_GLX_CALL do { } while(0)
#define AFTER_GLX_CALL do { } while(0)

#endif
    
void 
GLXLibrary::xDestroyContext(Display* display, GLXContext context)
{
    BEFORE_GLX_CALL;
    xDestroyContextInternal(display, context);
    AFTER_GLX_CALL;
}

Bool 
GLXLibrary::xMakeCurrent(Display* display, 
                         GLXDrawable drawable, 
                         GLXContext context)
{
    BEFORE_GLX_CALL;
    Bool result = xMakeCurrentInternal(display, drawable, context);
    AFTER_GLX_CALL;
    return result;
}

GLXContext 
GLXLibrary::xGetCurrentContext()
{
    BEFORE_GLX_CALL;
    GLXContext result = xGetCurrentContextInternal();
    AFTER_GLX_CALL;
    return result;
}

 void* 
GLXLibrary::xGetProcAddress(const char *procName)
{
    BEFORE_GLX_CALL;
    void* result = sGLXLibrary[gCurrLib].xGetProcAddressInternal(procName);
    AFTER_GLX_CALL;
    return result;
}

GLXFBConfig*
GLXLibrary::xChooseFBConfig(Display* display, 
                            int screen, 
                            const int *attrib_list, 
                            int *nelements)
{
    BEFORE_GLX_CALL;
    GLXFBConfig* result = xChooseFBConfigInternal(display, screen, attrib_list, nelements);
    AFTER_GLX_CALL;
    return result;
}

GLXFBConfig* 
GLXLibrary::xGetFBConfigs(Display* display, 
                          int screen, 
                          int *nelements)
{
    BEFORE_GLX_CALL;
    GLXFBConfig* result = xGetFBConfigsInternal(display, screen, nelements);
    AFTER_GLX_CALL;
    return result;
}
    
GLXContext
GLXLibrary::xCreateNewContext(Display* display, 
                              GLXFBConfig config, 
                              int render_type, 
                              GLXContext share_list, 
                              Bool direct)
{
    BEFORE_GLX_CALL;
    GLXContext result = xCreateNewContextInternal(display, config, 
                                                  render_type,
                                                  share_list, direct);
    AFTER_GLX_CALL;
    return result;
}

int
GLXLibrary::xGetFBConfigAttrib(Display *display,
                               GLXFBConfig config,
                               int attribute,
                               int *value)
{
    BEFORE_GLX_CALL;
    int result = xGetFBConfigAttribInternal(display, config,
                                            attribute, value);
    AFTER_GLX_CALL;
    return result;
}

void
GLXLibrary::xSwapBuffers(Display *display, GLXDrawable drawable)
{
    BEFORE_GLX_CALL;
    xSwapBuffersInternal(display, drawable);
    AFTER_GLX_CALL;
}

const char *
GLXLibrary::xQueryExtensionsString(Display *display,
                                   int screen)
{
    BEFORE_GLX_CALL;
    const char *result = xQueryExtensionsStringInternal(display, screen);
    AFTER_GLX_CALL;
    return result;
}

const char *
GLXLibrary::xGetClientString(Display *display,
                             int screen)
{
    BEFORE_GLX_CALL;
    const char *result = xGetClientStringInternal(display, screen);
    AFTER_GLX_CALL;
    return result;
}

const char *
GLXLibrary::xQueryServerString(Display *display,
                               int screen, int name)
{
    BEFORE_GLX_CALL;
    const char *result = xQueryServerStringInternal(display, screen, name);
    AFTER_GLX_CALL;
    return result;
}

GLXPixmap
GLXLibrary::xCreatePixmap(Display *display, 
                          GLXFBConfig config,
                          Pixmap pixmap,
                          const int *attrib_list)
{
    BEFORE_GLX_CALL;
    GLXPixmap result = xCreatePixmapInternal(display, config,
                                             pixmap, attrib_list);
    AFTER_GLX_CALL;
    return result;
}

GLXPixmap
GLXLibrary::xCreateGLXPixmapWithConfig(Display *display,
                                       GLXFBConfig config,
                                       Pixmap pixmap)
{
    BEFORE_GLX_CALL;
    GLXPixmap result = xCreateGLXPixmapWithConfigInternal(display, config, pixmap);
    AFTER_GLX_CALL;
    return result;
}

void
GLXLibrary::xDestroyPixmap(Display *display, GLXPixmap pixmap)
{
    BEFORE_GLX_CALL;
    xDestroyPixmapInternal(display, pixmap);
    AFTER_GLX_CALL;
}

Bool
GLXLibrary::xQueryVersion(Display *display,
                          int *major,
                          int *minor)
{
    BEFORE_GLX_CALL;
    Bool result = xQueryVersionInternal(display, major, minor);
    AFTER_GLX_CALL;
    return result;
}

void
GLXLibrary::xBindTexImage(Display *display,
                          GLXDrawable drawable,
                          int buffer,
                          const int *attrib_list)
{
    BEFORE_GLX_CALL;
    xBindTexImageInternal(display, drawable, buffer, attrib_list);
    AFTER_GLX_CALL;
}

void
GLXLibrary::xReleaseTexImage(Display *display,
                             GLXDrawable drawable,
                             int buffer)
{
    BEFORE_GLX_CALL;
    xReleaseTexImageInternal(display, drawable, buffer);
    AFTER_GLX_CALL;
}

void 
GLXLibrary::xWaitGL()
{
    BEFORE_GLX_CALL;
    xWaitGLInternal();
    AFTER_GLX_CALL;
}

void
GLXLibrary::xWaitX()
{
    BEFORE_GLX_CALL;
    xWaitXInternal();
    AFTER_GLX_CALL;
}

GLXContext
GLXLibrary::xCreateContextAttribs(Display* display, 
                                  GLXFBConfig config, 
                                  GLXContext share_list, 
                                  Bool direct,
                                  const int* attrib_list)
{
    BEFORE_GLX_CALL;
    GLXContext result = xCreateContextAttribsInternal(display, 
                                                      config, 
                                                      share_list, 
                                                      direct,
                                                      attrib_list);
    AFTER_GLX_CALL;
    return result;
}

class GLContextGLX : public GLContext
{
public:
    static already_AddRefed<GLContextGLX>
    CreateGLContext(const SurfaceCaps& caps,
                    GLContextGLX* shareContext,
                    bool isOffscreen,
                    Display* display,
                    GLXDrawable drawable,
                    GLXFBConfig cfg,
                    bool deleteDrawable,
                    LibType libType = GLXLibrary::OPENGL_LIB,
                    gfxXlibSurface* pixmap = nullptr)
    {
        GLXLibrary& glx = sGLXLibrary[libType];

        int db = 0;
        int err = glx.xGetFBConfigAttrib(display, cfg,
                                         GLX_DOUBLEBUFFER, &db);
        if (GLX_BAD_ATTRIBUTE != err) {
#ifdef DEBUG
            if (DebugMode()) {
                printf("[GLX] FBConfig is %sdouble-buffered\n", db ? "" : "not ");
            }
#endif
        }

        GLXContext context;
        nsRefPtr<GLContextGLX> glContext;
        bool error;

        ScopedXErrorHandler xErrorHandler;

TRY_AGAIN_NO_SHARING:

        error = false;

        GLXContext glxContext = shareContext ? shareContext->mContext : nullptr;
        if (glx.HasRobustness()) {
            int attrib_list[] = {
                LOCAL_GL_CONTEXT_FLAGS_ARB, LOCAL_GL_CONTEXT_ROBUST_ACCESS_BIT_ARB,
                LOCAL_GL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, LOCAL_GL_LOSE_CONTEXT_ON_RESET_ARB,
                0,
            };

            context = glx.xCreateContextAttribs(
                display,
                cfg,
                glxContext,
                True,
                attrib_list);
        } else {
            context = glx.xCreateNewContext(
                display,
                cfg,
                GLX_RGBA_TYPE,
                glxContext,
                True);
        }

        if (context) {
            glContext = new GLContextGLX(caps,
                                         shareContext,
                                         isOffscreen,
                                         display,
                                         drawable,
                                         context,
                                         deleteDrawable,
                                         db,
                                         pixmap,
                                         libType);
            if (!glContext->Init())
                error = true;
        } else {
            error = true;
        }

        error |= xErrorHandler.SyncAndGetError(display);

        if (error) {
            if (shareContext) {
                shareContext = nullptr;
                goto TRY_AGAIN_NO_SHARING;
            }

            NS_WARNING("Failed to create GLXContext!");
            glContext = nullptr; 
                                
        }

        return glContext.forget();
    }

    ~GLContextGLX()
    {
        MarkDestroyed();

        
#ifdef DEBUG
        bool success =
#endif
        mGLX->xMakeCurrent(mDisplay, None, nullptr);
        NS_ABORT_IF_FALSE(success,
            "glXMakeCurrent failed to release GL context before we call glXDestroyContext!");

        mGLX->xDestroyContext(mDisplay, mContext);

        if (mDeleteDrawable) {
            mGLX->xDestroyPixmap(mDisplay, mDrawable);
        }
    }

    GLContextType GetContextType() {
        return ContextTypeGLX;
    }

    bool Init()
    {
        MakeCurrent();
        SetupLookupFunction();
        if (!InitWithPrefix("gl", true)) {
            return false;
        }

        if (!IsExtensionSupported(EXT_framebuffer_object))
            return false;

        return true;
    }

    bool MakeCurrentImpl(bool aForce = false)
    {
        bool succeeded = true;

        
        
        
        
        
        
        if (aForce || mGLX->xGetCurrentContext() != mContext) {
            succeeded = mGLX->xMakeCurrent(mDisplay, mDrawable, mContext);
            NS_ASSERTION(succeeded, "Failed to make GL context current!");
        }

        return succeeded;
    }

    virtual bool IsCurrent() {
        return mGLX->xGetCurrentContext() == mContext;
    }

    bool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)&GLXLibrary::xGetProcAddress;
        return true;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch(aType) {
        case NativeGLContext:
            return mContext;
 
        case NativeThebesSurface:
            return mPixmap;

        default:
            return nullptr;
        }
    }

    bool IsDoubleBuffered()
    {
        return mDoubleBuffered;
    }

    bool SupportsRobustness()
    {
        return mGLX->HasRobustness();
    }

    bool SwapBuffers()
    {
        if (!mDoubleBuffered)
            return false;
        mGLX->xSwapBuffers(mDisplay, mDrawable);
        mGLX->xWaitGL();
        return true;
    }

    bool TextureImageSupportsGetBackingSurface()
    {
        return false;
    }

    virtual already_AddRefed<TextureImage>
    CreateTextureImage(const nsIntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLenum aWrapMode,
                       TextureImage::Flags aFlags = TextureImage::NoFlags,
                       TextureImage::ImageFormat aImageFormat = gfxImageFormatUnknown);

private:
    friend class GLContextProviderGLX;

    GLContextGLX(const SurfaceCaps& caps,
                 GLContext* shareContext,
                 bool isOffscreen,
                 Display *aDisplay,
                 GLXDrawable aDrawable,
                 GLXContext aContext,
                 bool aDeleteDrawable,
                 bool aDoubleBuffered,
                 gfxXlibSurface *aPixmap,
                 LibType libType)
        : GLContext(caps, shareContext, isOffscreen),
          mContext(aContext),
          mDisplay(aDisplay),
          mDrawable(aDrawable),
          mDeleteDrawable(aDeleteDrawable),
          mDoubleBuffered(aDoubleBuffered),
          mLibType(libType),
          mGLX(&sGLXLibrary[libType]),
          mPixmap(aPixmap)
    {
        MOZ_ASSERT(mGLX);
        
        SetProfileVersion(ContextProfile::OpenGLCompatibility, 200);
    }

    GLXContext mContext;
    Display *mDisplay;
    GLXDrawable mDrawable;
    bool mDeleteDrawable;
    bool mDoubleBuffered;

    LibType mLibType;
    GLXLibrary* mGLX;

    nsRefPtr<gfxXlibSurface> mPixmap;
};

class TextureImageGLX : public TextureImage
{
    friend already_AddRefed<TextureImage>
    GLContextGLX::CreateTextureImage(const nsIntSize&,
                                     ContentType,
                                     GLenum,
                                     TextureImage::Flags,
                                     TextureImage::ImageFormat);

public:
    virtual ~TextureImageGLX()
    {
        mGLContext->MakeCurrent();
        mGLContext->fDeleteTextures(1, &mTexture);
        sGLXLib.DestroyPixmap(mPixmap);
    }

    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion)
    {
        mInUpdate = true;
        return mUpdateSurface;
    }

    virtual void EndUpdate()
    {
        mInUpdate = false;
    }


    virtual bool DirectUpdate(gfxASurface* aSurface, const nsIntRegion& aRegion, const nsIntPoint& aFrom)
    {
        nsRefPtr<gfxContext> ctx = new gfxContext(mUpdateSurface);
        gfxUtils::ClipToRegion(ctx, aRegion);
        ctx->SetSource(aSurface, aFrom);
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->Paint();
        return true;
    }

    virtual void BindTexture(GLenum aTextureUnit)
    {
        mGLContext->fActiveTexture(aTextureUnit);
        mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
        sGLXLib.BindTexImage(mPixmap);
        mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
    }

    virtual void ReleaseTexture()
    {
        sGLXLib.ReleaseTexImage(mPixmap);
    }

    virtual already_AddRefed<gfxASurface> GetBackingSurface()
    {
        nsRefPtr<gfxASurface> copy = mUpdateSurface;
        return copy.forget();
    }

    virtual bool InUpdate() const { return mInUpdate; }

    virtual GLuint GetTextureID() {
        return mTexture;
    }

private:
   TextureImageGLX(GLuint aTexture,
                   const nsIntSize& aSize,
                   GLenum aWrapMode,
                   ContentType aContentType,
                   GLContext* aContext,
                   gfxASurface* aSurface,
                   GLXPixmap aPixmap,
                   TextureImage::Flags aFlags,
                   LibType aLibType)
        : TextureImage(aSize, aWrapMode, aContentType, aFlags)
        , mGLContext(aContext)
        , mUpdateSurface(aSurface)
        , mPixmap(aPixmap)
        , mInUpdate(false)
        , mTexture(aTexture)
        , sGLXLib(sGLXLibrary[aLibType])
    {
        if (aSurface->GetContentType() == GFX_CONTENT_COLOR_ALPHA) {
            mTextureFormat = FORMAT_R8G8B8A8;
        } else {
            mTextureFormat = FORMAT_R8G8B8X8;
        }
    }

    nsRefPtr<GLContext> mGLContext;
    nsRefPtr<gfxASurface> mUpdateSurface;
    GLXPixmap mPixmap;
    bool mInUpdate;
    GLuint mTexture;
    GLXLibrary& sGLXLib;

    virtual void ApplyFilter()
    {
        mGLContext->ApplyFilterToBoundTexture(mFilter);
    }
};

already_AddRefed<TextureImage>
GLContextGLX::CreateTextureImage(const nsIntSize& aSize,
                                 TextureImage::ContentType aContentType,
                                 GLenum aWrapMode,
                                 TextureImage::Flags aFlags,
                                 TextureImage::ImageFormat aImageFormat)
{
    if (!TextureImageSupportsGetBackingSurface()) {
        return GLContext::CreateTextureImage(aSize, 
                                             aContentType, 
                                             aWrapMode, 
                                             aFlags,
                                             aImageFormat);
    }

    Display *display = DefaultXDisplay();
    int xscreen = DefaultScreen(display);
    gfxImageFormat imageFormat =
        gfxPlatform::GetPlatform()->OptimalFormatForContent(aContentType);

    XRenderPictFormat* xrenderFormat =
        gfxXlibSurface::FindRenderFormat(display, imageFormat);
    NS_ASSERTION(xrenderFormat, "Could not find a render format for our display!");


    nsRefPtr<gfxXlibSurface> surface =
        gfxXlibSurface::Create(ScreenOfDisplay(display, xscreen),
                               xrenderFormat,
                               gfxIntSize(aSize.width, aSize.height));

    NS_ASSERTION(surface, "Failed to create xlib surface!");

    if (aContentType == GFX_CONTENT_COLOR_ALPHA) {
        nsRefPtr<gfxContext> ctx = new gfxContext(surface);
        ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
        ctx->Paint();
    }

    MakeCurrent();
    GLXPixmap pixmap = mGLX->CreatePixmap(surface);
    
    
    if (!pixmap && imageFormat == gfxImageFormatA8) {
        return GLContext::CreateTextureImage(aSize,
                                             aContentType,
                                             aWrapMode,
                                             aFlags,
                                             aImageFormat);
    }
    NS_ASSERTION(pixmap, "Failed to create pixmap!");

    GLuint texture;
    fGenTextures(1, &texture);

    fActiveTexture(LOCAL_GL_TEXTURE0);
    fBindTexture(LOCAL_GL_TEXTURE_2D, texture);

    nsRefPtr<TextureImageGLX> teximage =
        new TextureImageGLX(texture, aSize, aWrapMode, aContentType, 
                            this, surface, pixmap, aFlags, mLibType);

    GLint texfilter = aFlags & TextureImage::UseNearestFilter ? LOCAL_GL_NEAREST : LOCAL_GL_LINEAR;
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, texfilter);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, texfilter);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, aWrapMode);
    fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, aWrapMode);

    return teximage.forget();
}

static GLContextGLX *
GetGlobalContextGLX(const ContextFlags aFlags = ContextFlagsNone)
{
    return static_cast<GLContextGLX*>(GLContextProviderGLX::GetGlobalContext(aFlags));
}

static bool
AreCompatibleVisuals(Visual *one, Visual *two)
{
    if (one->c_class != two->c_class) {
        return false;
    }

    if (one->red_mask != two->red_mask ||
        one->green_mask != two->green_mask ||
        one->blue_mask != two->blue_mask) {
        return false;
    }

    if (one->bits_per_rgb != two->bits_per_rgb) {
        return false;
    }

    return true;
}

already_AddRefed<GLContext>
GLContextProviderGLX::CreateForWindow(nsIWidget *aWidget)
{
    const LibType libType = GLXLibrary::OPENGL_LIB;
    if (!sDefGLXLib.EnsureInitialized(libType)) {
        return nullptr;
    }

    
    
    
    
    
    

    Display *display = (Display*)aWidget->GetNativeData(NS_NATIVE_DISPLAY); 
    int xscreen = DefaultScreen(display);
    Window window = GET_NATIVE_WINDOW(aWidget);

    int numConfigs;
    ScopedXFree<GLXFBConfig> cfgs;
    if (sDefGLXLib.IsATI() || 
        !sDefGLXLib.GLXVersionCheck(1, 3)) {
        const int attribs[] = {
            GLX_DOUBLEBUFFER, False,
            0
        };
        cfgs = sDefGLXLib.xChooseFBConfig(display,
                                       xscreen,
                                       attribs,
                                       &numConfigs);
    } else {
        cfgs = sDefGLXLib.xGetFBConfigs(display,
                                     xscreen,
                                     &numConfigs);
    }

    if (!cfgs) {
        NS_WARNING("[GLX] glXGetFBConfigs() failed");
        return nullptr;
    }
    NS_ASSERTION(numConfigs > 0, "No FBConfigs found!");

    
    
    
    XWindowAttributes widgetAttrs;
    if (!XGetWindowAttributes(display, window, &widgetAttrs)) {
        NS_WARNING("[GLX] XGetWindowAttributes() failed");
        return nullptr;
    }
    const VisualID widgetVisualID = XVisualIDFromVisual(widgetAttrs.visual);
#ifdef DEBUG
    printf("[GLX] widget has VisualID 0x%lx\n", widgetVisualID);
#endif

    int matchIndex = -1;

    for (int i = 0; i < numConfigs; i++) {
        int visid = None;
        sDefGLXLib.xGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &visid);
        if (!visid) {
            continue;
        }
        if (sDefGLXLib.IsATI()) {
            int depth;
            Visual *visual;
            FindVisualAndDepth(display, visid, &visual, &depth);
            if (depth == widgetAttrs.depth &&
                AreCompatibleVisuals(widgetAttrs.visual, visual)) {
                matchIndex = i;
                break;
            }
        } else {
            if (widgetVisualID == static_cast<VisualID>(visid)) {
                matchIndex = i;
                break;
            }
        }
    }

    if (matchIndex == -1) {
        NS_WARNING("[GLX] Couldn't find a FBConfig matching widget visual");
        return nullptr;
    }

    GLContextGLX *shareContext = GetGlobalContextGLX();

    SurfaceCaps caps = SurfaceCaps::Any();
    nsRefPtr<GLContextGLX> glContext = GLContextGLX::CreateGLContext(caps,
                                                                     shareContext,
                                                                     false,
                                                                     display,
                                                                     window,
                                                                     cfgs[matchIndex],
                                                                     false,
                                                                     libType);

    return glContext.forget();
}

static already_AddRefed<GLContextGLX>
CreateOffscreenPixmapContext(const gfxIntSize& size, LibType libToUse)
{
    GLXLibrary& glx = sGLXLibrary[libToUse];
    if (!glx.EnsureInitialized(libToUse)) {
        return nullptr;
    }

    Display *display = DefaultXDisplay();
    int xscreen = DefaultScreen(display);

    int attribs[] = {
        GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
        GLX_X_RENDERABLE, True,
        0
    };
    int numConfigs = 0;

    ScopedXFree<GLXFBConfig> cfgs;
    cfgs = glx.xChooseFBConfig(display,
                               xscreen,
                               attribs,
                               &numConfigs);
    if (!cfgs) {
        return nullptr;
    }

    MOZ_ASSERT(numConfigs > 0,
               "glXChooseFBConfig() failed to match our requested format and violated its spec!");

    int visid = None;
    int chosenIndex = 0;

    for (int i = 0; i < numConfigs; ++i) {
        int dtype;

        if (glx.xGetFBConfigAttrib(display, cfgs[i], GLX_DRAWABLE_TYPE, &dtype) != Success
            || !(dtype & GLX_PIXMAP_BIT))
        {
            continue;
        }
        if (glx.xGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &visid) != Success
            || visid == 0)
        {
            continue;
        }

        chosenIndex = i;
        break;
    }

    if (!visid) {
        NS_WARNING("glXChooseFBConfig() didn't give us any configs with visuals!");
        return nullptr;
    }

    Visual *visual;
    int depth;
    FindVisualAndDepth(display, visid, &visual, &depth);
    ScopedXErrorHandler xErrorHandler;
    GLXPixmap glxpixmap = 0;
    bool error = false;

    gfxIntSize dummySize(16, 16);
    nsRefPtr<gfxXlibSurface> xsurface = gfxXlibSurface::Create(DefaultScreenOfDisplay(display),
                                                               visual,
                                                               dummySize);
    if (xsurface->CairoStatus() != 0) {
        error = true;
        goto DONE_CREATING_PIXMAP;
    }

    
    
    
    if (glx.GLXVersionCheck(1, 3)) {
        glxpixmap = glx.xCreatePixmap(display,
                                          cfgs[chosenIndex],
                                          xsurface->XDrawable(),
                                          nullptr);
    } else {
        glxpixmap = glx.xCreateGLXPixmapWithConfig(display,
                                                       cfgs[chosenIndex],
                                                       xsurface->
                                                       XDrawable());
    }
    if (glxpixmap == 0) {
        error = true;
    }

DONE_CREATING_PIXMAP:

    nsRefPtr<GLContextGLX> glContext;
    bool serverError = xErrorHandler.SyncAndGetError(display);

    if (!error && 
        !serverError)
    {
        ContextFlags flag = libToUse == GLXLibrary::MESA_LLVMPIPE_LIB
                                         ? ContextFlagsMesaLLVMPipe
                                         : ContextFlagsNone;
        
        SurfaceCaps dummyCaps = SurfaceCaps::Any();
        GLContextGLX* shareContext = GetGlobalContextGLX(flag);

        glContext = GLContextGLX::CreateGLContext(dummyCaps,
                                                  shareContext,
                                                  true,
                                                  display,
                                                  glxpixmap,
                                                  cfgs[chosenIndex],
                                                  true,
                                                  libToUse,
                                                  xsurface);
    }

    return glContext.forget();
}

already_AddRefed<GLContext>
GLContextProviderGLX::CreateOffscreen(const gfxIntSize& size,
                                      const SurfaceCaps& caps,
                                      ContextFlags flags)
{
    LibType libType = GLXLibrary::SelectLibrary(flags);
    gCurrLib = libType;

    gfxIntSize dummySize = gfxIntSize(16, 16);
    nsRefPtr<GLContextGLX> glContext =
        CreateOffscreenPixmapContext(dummySize, libType);

    if (!glContext)
        return nullptr;

    if (!glContext->InitOffscreen(size, caps))
        return nullptr;

    return glContext.forget();
}

SharedTextureHandle
GLContextProviderGLX::CreateSharedHandle(SharedTextureShareType shareType,
                                         void* buffer,
                                         SharedTextureBufferType bufferType)
{
  return 0;
}

already_AddRefed<gfxASurface>
GLContextProviderGLX::GetSharedHandleAsSurface(SharedTextureShareType shareType,
                                               SharedTextureHandle sharedHandle)
{
  return nullptr;
}

static nsRefPtr<GLContext> gGlobalContext[GLXLibrary::LIBS_MAX];

static bool gUseContextSharing = getenv("MOZ_DISABLE_CONTEXT_SHARING_GLX") == 0;

GLContext*
GLContextProviderGLX::GetGlobalContext(const ContextFlags aFlag)
{
    
    if (!gUseContextSharing) {
        return nullptr;
    }

    LibType libType = GLXLibrary::SelectLibrary(aFlag);
    static bool triedToCreateContext[GLXLibrary::LIBS_MAX] = {false, false};
    if (!triedToCreateContext[libType] && !gGlobalContext[libType]) {
        triedToCreateContext[libType] = true;

        gfxIntSize dummySize = gfxIntSize(16, 16);
        gGlobalContext[libType] = CreateOffscreenPixmapContext(dummySize, libType);
        if (gGlobalContext[libType])
            gGlobalContext[libType]->SetIsGlobalSharedContext(true);
    }

    return gGlobalContext[libType];
}

void
GLContextProviderGLX::Shutdown()
{
    for (int i = 0; i < GLXLibrary::LIBS_MAX; ++i)
      gGlobalContext[i] = nullptr;
}

} 
} 

