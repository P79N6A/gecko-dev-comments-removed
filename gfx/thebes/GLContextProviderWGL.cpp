



































#include "GLContextProvider.h"
#include "GLContext.h"
#include "nsDebug.h"
#include "nsIWidget.h"
#include "WGLLibrary.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxWindowsSurface.h"

#include "gfxCrashReporterUtils.h"

#include "prenv.h"

#include "mozilla/Preferences.h"

namespace mozilla {
namespace gl {

WGLLibrary sWGLLibrary;

static HWND gSharedWindow = 0;
static HDC gSharedWindowDC = 0;
static HGLRC gSharedWindowGLContext = 0;
static int gSharedWindowPixelFormat = 0;

static bool gUseDoubleBufferedWindows = false;

static HWND
CreateDummyWindow(HDC *aWindowDC = nsnull)
{
    WNDCLASSW wc;
    if (!GetClassInfoW(GetModuleHandle(NULL), L"GLContextWGLClass", &wc)) {
        ZeroMemory(&wc, sizeof(WNDCLASSW));
        wc.style = CS_OWNDC;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = L"GLContextWGLClass";
        if (!RegisterClassW(&wc)) {
            NS_WARNING("Failed to register GLContextWGLClass?!");
            
            return NULL;
        }
    }

    HWND win = CreateWindowW(L"GLContextWGLClass", L"GLContextWGL", 0,
                             0, 0, 16, 16,
                             NULL, NULL, GetModuleHandle(NULL), NULL);
    NS_ENSURE_TRUE(win, NULL);

    HDC dc = GetDC(win);
    NS_ENSURE_TRUE(dc, NULL);

    if (gSharedWindowPixelFormat == 0) {
        PIXELFORMATDESCRIPTOR pfd;
        ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        if (gUseDoubleBufferedWindows)
            pfd.dwFlags |= PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cRedBits = 8;
        pfd.cGreenBits = 8;
        pfd.cBlueBits = 8;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 0;
        pfd.iLayerType = PFD_MAIN_PLANE;

        gSharedWindowPixelFormat = ChoosePixelFormat(dc, &pfd);
    }

    if (!gSharedWindowPixelFormat ||
        !SetPixelFormat(dc, gSharedWindowPixelFormat, NULL))
    {
        NS_WARNING("SetPixelFormat failed!");
        DestroyWindow(win);
        return NULL;
    }

    if (aWindowDC) {
        *aWindowDC = dc;
    }

    return win;
}

bool
WGLLibrary::EnsureInitialized()
{
    if (mInitialized)
        return true;

    mozilla::ScopedGfxFeatureReporter reporter("WGL");

    if (!mOGLLibrary) {
        mOGLLibrary = PR_LoadLibrary("Opengl32.dll");
        if (!mOGLLibrary) {
            NS_WARNING("Couldn't load OpenGL DLL.");
            return false;
        }
    }

    gUseDoubleBufferedWindows = PR_GetEnv("MOZ_WGL_DB") != nsnull;

    LibrarySymbolLoader::SymLoadStruct earlySymbols[] = {
        { (PRFuncPtr*) &fCreateContext, { "wglCreateContext", NULL } },
        { (PRFuncPtr*) &fMakeCurrent, { "wglMakeCurrent", NULL } },
        { (PRFuncPtr*) &fGetProcAddress, { "wglGetProcAddress", NULL } },
        { (PRFuncPtr*) &fDeleteContext, { "wglDeleteContext", NULL } },
        { (PRFuncPtr*) &fGetCurrentContext, { "wglGetCurrentContext", NULL } },
        { (PRFuncPtr*) &fGetCurrentDC, { "wglGetCurrentDC", NULL } },
        { (PRFuncPtr*) &fShareLists, { "wglShareLists", NULL } },
        { NULL, { NULL } }
    };

    if (!LibrarySymbolLoader::LoadSymbols(mOGLLibrary, &earlySymbols[0])) {
        NS_WARNING("Couldn't find required entry points in OpenGL DLL (early init)");
        return false;
    }

    
    
    gSharedWindow = CreateDummyWindow(&gSharedWindowDC);
    NS_ENSURE_TRUE(gSharedWindow, false);

    
    gSharedWindowGLContext = fCreateContext(gSharedWindowDC);
    NS_ENSURE_TRUE(gSharedWindowGLContext, false);

    HGLRC curCtx = fGetCurrentContext();
    HDC curDC = fGetCurrentDC();

    if (!fMakeCurrent((HDC)gSharedWindowDC, (HGLRC)gSharedWindowGLContext)) {
        NS_WARNING("wglMakeCurrent failed");
        return false;
    }

    
    

    LibrarySymbolLoader::SymLoadStruct pbufferSymbols[] = {
        { (PRFuncPtr*) &fCreatePbuffer, { "wglCreatePbufferARB", "wglCreatePbufferEXT", NULL } },
        { (PRFuncPtr*) &fDestroyPbuffer, { "wglDestroyPbufferARB", "wglDestroyPbufferEXT", NULL } },
        { (PRFuncPtr*) &fGetPbufferDC, { "wglGetPbufferDCARB", "wglGetPbufferDCEXT", NULL } },
        { (PRFuncPtr*) &fBindTexImage, { "wglBindTexImageARB", "wglBindTexImageEXT", NULL } },
        { (PRFuncPtr*) &fReleaseTexImage, { "wglReleaseTexImageARB", "wglReleaseTexImageEXT", NULL } },
        { NULL, { NULL } }
    };

    LibrarySymbolLoader::SymLoadStruct pixFmtSymbols[] = {
        { (PRFuncPtr*) &fChoosePixelFormat, { "wglChoosePixelFormatARB", "wglChoosePixelFormatEXT", NULL } },
        { (PRFuncPtr*) &fGetPixelFormatAttribiv, { "wglGetPixelFormatAttribivARB", "wglGetPixelFormatAttribivEXT", NULL } },
        { NULL, { NULL } }
    };

    if (!LibrarySymbolLoader::LoadSymbols(mOGLLibrary, &pbufferSymbols[0],
         (LibrarySymbolLoader::PlatformLookupFunction)fGetProcAddress))
    {
        
        fCreatePbuffer = nsnull;
    }

    if (!LibrarySymbolLoader::LoadSymbols(mOGLLibrary, &pixFmtSymbols[0],
         (LibrarySymbolLoader::PlatformLookupFunction)fGetProcAddress))
    {
        
        fChoosePixelFormat = nsnull;
    }

    
    fMakeCurrent(curDC, curCtx);

    mInitialized = true;

    
    
    
    if (GLContextProviderWGL::GetGlobalContext() == nsnull) {
        mInitialized = false;
        return false;
    }

    reporter.SetSuccessful();
    return true;
}

class GLContextWGL : public GLContext
{
public:
    GLContextWGL(const ContextFormat& aFormat,
                 GLContext *aSharedContext,
                 HDC aDC,
                 HGLRC aContext,
                 HWND aWindow = nsnull,
                 bool aIsOffscreen = false)
        : GLContext(aFormat, aIsOffscreen, aSharedContext),
          mDC(aDC),
          mContext(aContext),
          mWnd(aWindow),
          mPBuffer(NULL),
          mPixelFormat(0),
          mIsDoubleBuffered(false)
    {
    }

    GLContextWGL(const ContextFormat& aFormat,
                 GLContext *aSharedContext,
                 HANDLE aPbuffer,
                 HDC aDC,
                 HGLRC aContext,
                 int aPixelFormat)
        : GLContext(aFormat, true, aSharedContext),
          mDC(aDC),
          mContext(aContext),
          mWnd(NULL),
          mPBuffer(aPbuffer),
          mPixelFormat(aPixelFormat),
          mIsDoubleBuffered(false)
    {
    }

    ~GLContextWGL()
    {
        MarkDestroyed();

        sWGLLibrary.fDeleteContext(mContext);

        if (mPBuffer)
            sWGLLibrary.fDestroyPbuffer(mPBuffer);
        if (mWnd)
            DestroyWindow(mWnd);
    }

    GLContextType GetContextType() {
        return ContextTypeWGL;
    }

    bool Init()
    {
        if (!mDC || !mContext)
            return false;

        MakeCurrent();
        SetupLookupFunction();
        return InitWithPrefix("gl", true);
    }

    bool MakeCurrentImpl(bool aForce = false)
    {
        BOOL succeeded = true;

        
        
        
        
        if (aForce || sWGLLibrary.fGetCurrentContext() != mContext) {
            succeeded = sWGLLibrary.fMakeCurrent(mDC, mContext);
            NS_ASSERTION(succeeded, "Failed to make GL context current!");
        }

        return succeeded;
    }

    void SetIsDoubleBuffered(bool aIsDB) {
        mIsDoubleBuffered = aIsDB;
    }

    virtual bool IsDoubleBuffered() {
        return mIsDoubleBuffered;
    }

    virtual bool SwapBuffers() {
        if (!mIsDoubleBuffered)
            return false;
        return ::SwapBuffers(mDC);
    }

    bool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sWGLLibrary.fGetProcAddress;
        return true;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch (aType) {
        case NativeGLContext:
            return mContext;

        default:
            return nsnull;
        }
    }

    bool BindTex2DOffscreen(GLContext *aOffscreen);
    void UnbindTex2DOffscreen(GLContext *aOffscreen);
    bool ResizeOffscreen(const gfxIntSize& aNewSize);

    HGLRC Context() { return mContext; }

protected:
    friend class GLContextProviderWGL;

    HDC mDC;
    HGLRC mContext;
    HWND mWnd;
    HANDLE mPBuffer;
    int mPixelFormat;

    bool mIsDoubleBuffered;
};

bool
GLContextWGL::BindTex2DOffscreen(GLContext *aOffscreen)
{
    if (aOffscreen->GetContextType() != ContextTypeWGL) {
        NS_WARNING("non-WGL context");
        return false;
    }

    if (!aOffscreen->IsOffscreen()) {
        NS_WARNING("non-offscreen context");
        return false;
    }

    GLContextWGL *offs = static_cast<GLContextWGL*>(aOffscreen);

    if (offs->mPBuffer) {
        BOOL ok = sWGLLibrary.fBindTexImage(offs->mPBuffer,
                                            LOCAL_WGL_FRONT_LEFT_ARB);
        if (!ok) {
            NS_WARNING("CanvasLayerOGL::Updated wglBindTexImageARB failed");
            return false;
        }
    } else if (offs->mOffscreenTexture) {
        if (offs->GetSharedContext() != GLContextProviderWGL::GetGlobalContext())
        {
            NS_WARNING("offscreen FBO context can only be bound with context sharing!");
            return false;
        }

        fBindTexture(LOCAL_GL_TEXTURE_2D, offs->mOffscreenTexture);
    } else {
        NS_WARNING("don't know how to bind this!");
        return false;
    }

    return true;
}

void
GLContextWGL::UnbindTex2DOffscreen(GLContext *aOffscreen)
{
    NS_ASSERTION(aOffscreen->GetContextType() == ContextTypeWGL, "wrong type");

    GLContextWGL *offs = static_cast<GLContextWGL*>(aOffscreen);
    if (offs->mPBuffer) {
        
        
        
        sWGLLibrary.fReleaseTexImage(offs->mPBuffer, LOCAL_WGL_FRONT_LEFT_ARB);
    }
}


static bool
GetMaxSize(HDC hDC, int format, gfxIntSize& size)
{
    int query[] = {LOCAL_WGL_MAX_PBUFFER_WIDTH_ARB, LOCAL_WGL_MAX_PBUFFER_HEIGHT_ARB};
    int result[2];

    
    if (!sWGLLibrary.fGetPixelFormatAttribiv(hDC, format, 0, 2, query, result))
        return false;

    size.width = result[0];
    size.height = result[1];
    return true;
}

static bool
IsValidSizeForFormat(HDC hDC, int format, const gfxIntSize& requested)
{
    gfxIntSize max;
    if (!GetMaxSize(hDC, format, max))
        return true;

    if (requested.width > max.width)
        return false;
    if (requested.height > max.height)
        return false;

    return true;
}

bool
GLContextWGL::ResizeOffscreen(const gfxIntSize& aNewSize)
{
    if (mPBuffer) {
        if (!IsValidSizeForFormat(gSharedWindowDC, mPixelFormat, aNewSize))
            return false;

        int pbattrs[] = {
            LOCAL_WGL_TEXTURE_FORMAT_ARB,
              mCreationFormat.alpha > 0 ? LOCAL_WGL_TEXTURE_RGBA_ARB
                                        : LOCAL_WGL_TEXTURE_RGB_ARB,
            LOCAL_WGL_TEXTURE_TARGET_ARB, LOCAL_WGL_TEXTURE_2D_ARB,
            0
        };

        HANDLE newbuf = sWGLLibrary.fCreatePbuffer(gSharedWindowDC, mPixelFormat,
                                                   aNewSize.width, aNewSize.height,
                                                   pbattrs);
        if (!newbuf)
            return false;

        bool isCurrent = false;
        if (sWGLLibrary.fGetCurrentContext() == mContext) {
            sWGLLibrary.fMakeCurrent(NULL, NULL);
            isCurrent = true;
        }

        sWGLLibrary.fDestroyPbuffer(mPBuffer);

        mPBuffer = newbuf;
        mDC = sWGLLibrary.fGetPbufferDC(mPBuffer);

        mOffscreenSize = aNewSize;
        mOffscreenActualSize = aNewSize;

        MakeCurrent();
        ClearSafely();

        return ResizeOffscreenFBO(aNewSize, false);
    }

    return ResizeOffscreenFBO(aNewSize, true);
}

static GLContextWGL *
GetGlobalContextWGL()
{
    return static_cast<GLContextWGL*>(GLContextProviderWGL::GetGlobalContext());
}

already_AddRefed<GLContext>
GLContextProviderWGL::CreateForWindow(nsIWidget *aWidget)
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

    





    HDC dc = (HDC)aWidget->GetNativeData(NS_NATIVE_GRAPHIC);

    SetPixelFormat(dc, gSharedWindowPixelFormat, NULL);
    HGLRC context = sWGLLibrary.fCreateContext(dc);
    if (!context) {
        return nsnull;
    }

    GLContextWGL *shareContext = GetGlobalContextWGL();
    if (shareContext &&
        !sWGLLibrary.fShareLists(shareContext->Context(), context))
    {
        shareContext = nsnull;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(ContextFormat(ContextFormat::BasicRGB24),
                                                        shareContext, dc, context);
    if (!glContext->Init()) {
        return nsnull;
    }

    glContext->SetIsDoubleBuffered(gUseDoubleBufferedWindows);

    return glContext.forget();
}

static already_AddRefed<GLContextWGL>
CreatePBufferOffscreenContext(const gfxIntSize& aSize,
                              const ContextFormat& aFormat)
{
#define A1(_a,_x)  do { _a.AppendElement(_x); } while(0)
#define A2(_a,_x,_y)  do { _a.AppendElement(_x); _a.AppendElement(_y); } while(0)

    nsTArray<int> attrs;

    A2(attrs, LOCAL_WGL_SUPPORT_OPENGL_ARB, LOCAL_GL_TRUE);
    A2(attrs, LOCAL_WGL_DRAW_TO_PBUFFER_ARB, LOCAL_GL_TRUE);
    A2(attrs, LOCAL_WGL_DOUBLE_BUFFER_ARB, LOCAL_GL_FALSE);

    A2(attrs, LOCAL_WGL_ACCELERATION_ARB, LOCAL_WGL_FULL_ACCELERATION_ARB);

    A2(attrs, LOCAL_WGL_COLOR_BITS_ARB, aFormat.colorBits());
    A2(attrs, LOCAL_WGL_RED_BITS_ARB, aFormat.red);
    A2(attrs, LOCAL_WGL_GREEN_BITS_ARB, aFormat.green);
    A2(attrs, LOCAL_WGL_BLUE_BITS_ARB, aFormat.blue);
    A2(attrs, LOCAL_WGL_ALPHA_BITS_ARB, aFormat.alpha);

    A2(attrs, LOCAL_WGL_DEPTH_BITS_ARB, aFormat.depth);
    A2(attrs, LOCAL_WGL_STENCIL_BITS_ARB, aFormat.stencil);

    if (aFormat.alpha > 0) {
        A2(attrs, LOCAL_WGL_BIND_TO_TEXTURE_RGBA_ARB, LOCAL_GL_TRUE);
    } else {
        A2(attrs, LOCAL_WGL_BIND_TO_TEXTURE_RGB_ARB, LOCAL_GL_TRUE);
    }

    A2(attrs, LOCAL_WGL_DOUBLE_BUFFER_ARB, LOCAL_GL_FALSE);
    A2(attrs, LOCAL_WGL_STEREO_ARB, LOCAL_GL_FALSE);

    A1(attrs, 0);

    nsTArray<int> pbattrs;
    A2(pbattrs, LOCAL_WGL_TEXTURE_TARGET_ARB, LOCAL_WGL_TEXTURE_2D_ARB);

    if (aFormat.alpha > 0) {
        A2(pbattrs, LOCAL_WGL_TEXTURE_FORMAT_ARB, LOCAL_WGL_TEXTURE_RGBA_ARB);
    } else {
        A2(pbattrs, LOCAL_WGL_TEXTURE_FORMAT_ARB, LOCAL_WGL_TEXTURE_RGB_ARB);
    }
    A1(pbattrs, 0);

    UINT numFormats = 256;
    int formats[256];

    if (!sWGLLibrary.fChoosePixelFormat(gSharedWindowDC,
                                        attrs.Elements(), NULL,
                                        numFormats, formats, &numFormats)
        || numFormats == 0)
    {
        return nsnull;
    }

    
    int chosenFormat = formats[0];

    if (!IsValidSizeForFormat(gSharedWindowDC, chosenFormat, aSize))
        return nsnull;

    HANDLE pbuffer = sWGLLibrary.fCreatePbuffer(gSharedWindowDC, chosenFormat,
                                                aSize.width, aSize.height,
                                                pbattrs.Elements());
    if (!pbuffer) {
        return nsnull;
    }

    HDC pbdc = sWGLLibrary.fGetPbufferDC(pbuffer);
    NS_ASSERTION(pbdc, "expected a dc");

    HGLRC context = sWGLLibrary.fCreateContext(pbdc);
    if (!context) {
        sWGLLibrary.fDestroyPbuffer(pbuffer);
        return false;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(aFormat,
                                                        nsnull,
                                                        pbuffer,
                                                        pbdc,
                                                        context,
                                                        chosenFormat);

    return glContext.forget();
}

static already_AddRefed<GLContextWGL>
CreateWindowOffscreenContext(const ContextFormat& aFormat)
{
    
    GLContextWGL *shareContext = GetGlobalContextWGL();
    if (!shareContext) {
        return nsnull;
    }
    
    HDC dc;
    HWND win = CreateDummyWindow(&dc);
    if (!win) {
        return nsnull;
    }
    
    HGLRC context = sWGLLibrary.fCreateContext(dc);
    if (!context) {
        return nsnull;
    }

    if (!sWGLLibrary.fShareLists(shareContext->Context(), context)) {
        NS_WARNING("wglShareLists failed!");

        sWGLLibrary.fDeleteContext(context);
        DestroyWindow(win);
        return nsnull;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(aFormat, shareContext,
                                                        dc, context, win, true);

    return glContext.forget();
}

already_AddRefed<GLContext>
GLContextProviderWGL::CreateOffscreen(const gfxIntSize& aSize,
                                      const ContextFormat& aFormat)
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

    ContextFormat actualFormat(aFormat);
    

    nsRefPtr<GLContextWGL> glContext;

    
    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), nsnull);
    const bool preferFBOs = Preferences::GetBool("wgl.prefer-fbo", false);
    if (!preferFBOs &&
        sWGLLibrary.fCreatePbuffer &&
        sWGLLibrary.fChoosePixelFormat)
    {
        glContext = CreatePBufferOffscreenContext(aSize, actualFormat);
    }

    
    if (!glContext) {
        glContext = CreateWindowOffscreenContext(actualFormat);
    }

    if (!glContext ||
        !glContext->Init())
    {
        return nsnull;
    }

    if (!glContext->ResizeOffscreenFBO(aSize, !glContext->mPBuffer))
        return nsnull;

    glContext->mOffscreenSize = aSize;
    glContext->mOffscreenActualSize = aSize;

    return glContext.forget();
}

already_AddRefed<GLContext>
GLContextProviderWGL::CreateForNativePixmapSurface(gfxASurface *aSurface)
{
    return nsnull;
}

static nsRefPtr<GLContextWGL> gGlobalContext;

GLContext *
GLContextProviderWGL::GetGlobalContext()
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

    static bool triedToCreateContext = false;

    if (!triedToCreateContext && !gGlobalContext) {
        triedToCreateContext = true;

        
        gGlobalContext = new GLContextWGL(ContextFormat(ContextFormat::BasicRGB24), nsnull,
                                          gSharedWindowDC, gSharedWindowGLContext);
        if (!gGlobalContext->Init()) {
            NS_WARNING("Global context GLContext initialization failed?");
            gGlobalContext = nsnull;
            return false;
        }

        gGlobalContext->SetIsGlobalSharedContext(true);
    }

    return static_cast<GLContext*>(gGlobalContext);
}

void
GLContextProviderWGL::Shutdown()
{
    gGlobalContext = nsnull;
}

} 
} 
