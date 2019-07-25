



































#include "GLContextProvider.h"
#include "GLContext.h"
#include "nsDebug.h"
#include "nsIWidget.h"
#include "WGLLibrary.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxWindowsSurface.h"

namespace mozilla {
namespace gl {

GLContextProvider sGLContextProvider;
WGLLibrary sWGLLibrary;

static HWND gDummyWindow = 0;
static HDC gDummyWindowDC = 0;
static HANDLE gDummyWindowGLContext = 0;

PRBool
WGLLibrary::EnsureInitialized()
{
    if (mInitialized)
        return PR_TRUE;

    if (!mOGLLibrary) {
        mOGLLibrary = PR_LoadLibrary("Opengl32.dll");
        if (!mOGLLibrary) {
            NS_WARNING("Couldn't load OpenGL DLL.");
            return PR_FALSE;
        }
    }

    LibrarySymbolLoader::SymLoadStruct earlySymbols[] = {
        { (PRFuncPtr*) &fCreateContext, { "wglCreateContext", NULL } },
        { (PRFuncPtr*) &fMakeCurrent, { "wglMakeCurrent", NULL } },
        { (PRFuncPtr*) &fGetProcAddress, { "wglGetProcAddress", NULL } },
        { (PRFuncPtr*) &fDeleteContext, { "wglDeleteContext", NULL } },
        { (PRFuncPtr*) &fGetCurrentContext, { "wglGetCurrentContext", NULL } },
        { (PRFuncPtr*) &fGetCurrentDC, { "wglGetCurrentDC", NULL } },
        { NULL, { NULL } }
    };

    if (!LibrarySymbolLoader::LoadSymbols(mOGLLibrary, &earlySymbols[0])) {
        NS_WARNING("Couldn't find required entry points in OpenGL DLL (early init)");
        return PR_FALSE;
    }

    
    

    WNDCLASSW wc;
    if (!GetClassInfoW(GetModuleHandle(NULL), L"DummyGLWindowClass", &wc)) {
        ZeroMemory(&wc, sizeof(WNDCLASSW));
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = L"DummyGLWindowClass";
        if (!RegisterClassW(&wc)) {
            NS_WARNING("Failed to register DummyGLWindowClass?!");
            
            return PR_FALSE;
        }
    }

    gDummyWindow = CreateWindowW(L"DummyGLWindowClass", L"DummyGLWindow", 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                 CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!gDummyWindow) {
        NS_WARNING("CreateWindow DummyGLWindow failed");
        return PR_FALSE;
    }

    gDummyWindowDC = GetDC(gDummyWindow);
    if (!gDummyWindowDC) {
        NS_WARNING("GetDC gDummyWindow failed");
        return PR_FALSE;
    }

    
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    int pixelformat = ChoosePixelFormat(gDummyWindowDC, &pfd);

    
    if (!SetPixelFormat(gDummyWindowDC, pixelformat, &pfd)) {
        NS_WARNING("SetPixelFormat failed");
        return PR_FALSE;
    }

    
    gDummyWindowGLContext = fCreateContext(gDummyWindowDC);
    if (!gDummyWindowGLContext) {
        NS_WARNING("wglCreateContext failed");
        return PR_FALSE;
    }

    HGLRC curCtx = fGetCurrentContext();
    HDC curDC = fGetCurrentDC();

    if (!fMakeCurrent((HDC)gDummyWindowDC, (HGLRC)gDummyWindowGLContext)) {
        NS_WARNING("wglMakeCurrent failed");
        return PR_FALSE;
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

    mInitialized = PR_TRUE;
    return PR_TRUE;
}

class GLContextWGL : public GLContext
{
public:
    GLContextWGL(HDC aDC, HGLRC aContext)
        : mContext(aContext), mDC(aDC), mPBuffer(nsnull), mPixelFormat(-1)
    { }

    GLContextWGL(HANDLE aPBuffer, int aPixelFormat) {
        mPBuffer = aPBuffer;
        mPixelFormat = aPixelFormat;
        mDC = sWGLLibrary.fGetPbufferDC(mPBuffer);
        mContext = sWGLLibrary.fCreateContext(mDC);
    }

    ~GLContextWGL()
    {
        sWGLLibrary.fDeleteContext(mContext);

        if (mPBuffer)
            sWGLLibrary.fDestroyPbuffer(mPBuffer);
    }

    PRBool Init()
    {
        MakeCurrent();
        SetupLookupFunction();
        return InitWithPrefix("gl", PR_TRUE);
    }

    PRBool MakeCurrent()
    {
        BOOL succeeded = PR_TRUE;

        
        
        
        
        if (sWGLLibrary.fGetCurrentContext() != mContext) {
            succeeded = sWGLLibrary.fMakeCurrent(mDC, mContext);
            NS_ASSERTION(succeeded, "Failed to make GL context current!");
        }

        return succeeded;
    }

    PRBool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sWGLLibrary.fGetProcAddress;
        return PR_TRUE;
    }

    void *GetNativeData(NativeDataType aType)
    {
        switch (aType) {
        case NativeGLContext:
            return mContext;

        case NativePBuffer:
            return mPBuffer;

        default:
            return nsnull;
        }
    }

    PRBool Resize(const gfxIntSize& aNewSize) {
        if (!mPBuffer)
            return PR_FALSE;

        nsTArray<int> pbattribs;
        pbattribs.AppendElement(LOCAL_WGL_TEXTURE_FORMAT_ARB);
        
        if (true ) {
            pbattribs.AppendElement(LOCAL_WGL_TEXTURE_RGBA_ARB);
        } else {
            pbattribs.AppendElement(LOCAL_WGL_TEXTURE_RGB_ARB);
        }
        pbattribs.AppendElement(LOCAL_WGL_TEXTURE_TARGET_ARB);
        pbattribs.AppendElement(LOCAL_WGL_TEXTURE_2D_ARB);

        pbattribs.AppendElement(0);

        HANDLE newbuf = sWGLLibrary.fCreatePbuffer(gDummyWindowDC, mPixelFormat,
                                                   aNewSize.width, aNewSize.height,
                                                   pbattribs.Elements());
        if (!newbuf)
            return PR_FALSE;

        bool isCurrent = false;
        if (sWGLLibrary.fGetCurrentContext() == mContext) {
            sWGLLibrary.fMakeCurrent(NULL, NULL);
            isCurrent = true;
        }

        
        sWGLLibrary.fDestroyPbuffer(mPBuffer);

        mPBuffer = newbuf;
        mDC = sWGLLibrary.fGetPbufferDC(mPBuffer);

        if (isCurrent)
            MakeCurrent();

        return PR_TRUE;
    }

    virtual already_AddRefed<TextureImage>
    CreateBasicTextureImage(GLuint aTexture,
                            const nsIntSize& aSize,
                            TextureImage::ContentType aContentType,
                            GLContext* aContext);

private:
    HGLRC mContext;
    HDC mDC;
    HANDLE mPBuffer;
    int mPixelFormat;
};

class TextureImageWGL : public BasicTextureImage
{
    friend already_AddRefed<TextureImage>
    GLContextWGL::CreateBasicTextureImage(GLuint,
                                          const nsIntSize&,
                                          TextureImage::ContentType,
                                          GLContext*);

protected:
    virtual already_AddRefed<gfxASurface>
    CreateUpdateSurface(const gfxIntSize& aSize, ImageFormat aFmt)
    {
        return gfxPlatform::GetPlatform()->CreateOffscreenSurface(aSize, aFmt);
    }

    virtual already_AddRefed<gfxImageSurface>
    GetImageForUpload(gfxASurface* aUpdateSurface)
    {
        NS_ASSERTION(gfxASurface::SurfaceTypeWin32 == aUpdateSurface->GetType(),
                     "unexpected surface type");
        nsRefPtr<gfxImageSurface> uploadImage(
            static_cast<gfxWindowsSurface*>(aUpdateSurface)->
            GetImageSurface());
        return uploadImage.forget();
    }

private:
    TextureImageWGL(GLuint aTexture,
                    const nsIntSize& aSize,
                    ContentType aContentType,
                    GLContext* aContext)
        : BasicTextureImage(aTexture, aSize, aContentType, aContext)
    {}
};

already_AddRefed<TextureImage>
GLContextWGL::CreateBasicTextureImage(GLuint aTexture,
                                      const nsIntSize& aSize,
                                      TextureImage::ContentType aContentType,
                                      GLContext* aContext)
{
    nsRefPtr<TextureImageWGL> teximage(
        new TextureImageWGL(aTexture, aSize, aContentType, aContext));
    return teximage.forget();
}

already_AddRefed<GLContext>
GLContextProvider::CreateForWindow(nsIWidget *aWidget)
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }
    





    HDC dc = (HDC)aWidget->GetNativeData(NS_NATIVE_GRAPHIC);

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int iFormat = ChoosePixelFormat(dc, &pfd);

    SetPixelFormat(dc, iFormat, &pfd);
    HGLRC context = sWGLLibrary.fCreateContext(dc);
    if (!context) {
        return nsnull;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(dc, context);
    glContext->Init();

    return glContext.forget().get();
}

already_AddRefed<GLContext>
GLContextProvider::CreatePBuffer(const gfxIntSize& aSize, const ContextFormat& aFormat)
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }

    nsTArray<int> attribs;

#define A1_(_x)  do { attribs.AppendElement(_x); } while(0)
#define A2_(_x,_y)  do {                                                \
        attribs.AppendElement(_x);                                      \
        attribs.AppendElement(_y);                                      \
    } while(0)

    A2_(LOCAL_WGL_SUPPORT_OPENGL_ARB, LOCAL_GL_TRUE);
    A2_(LOCAL_WGL_DRAW_TO_PBUFFER_ARB, LOCAL_GL_TRUE);
    A2_(LOCAL_WGL_DOUBLE_BUFFER_ARB, LOCAL_GL_FALSE);

    A2_(LOCAL_WGL_ACCELERATION_ARB, LOCAL_WGL_FULL_ACCELERATION_ARB);

    A2_(LOCAL_WGL_COLOR_BITS_ARB, aFormat.colorBits());
    A2_(LOCAL_WGL_RED_BITS_ARB, aFormat.red);
    A2_(LOCAL_WGL_GREEN_BITS_ARB, aFormat.green);
    A2_(LOCAL_WGL_BLUE_BITS_ARB, aFormat.blue);
    A2_(LOCAL_WGL_ALPHA_BITS_ARB, aFormat.alpha);

    A2_(LOCAL_WGL_DEPTH_BITS_ARB, aFormat.depth);

    if (aFormat.alpha > 0)
        A2_(LOCAL_WGL_BIND_TO_TEXTURE_RGBA_ARB, LOCAL_GL_TRUE);
    else
        A2_(LOCAL_WGL_BIND_TO_TEXTURE_RGB_ARB, LOCAL_GL_TRUE);

    A2_(LOCAL_WGL_DOUBLE_BUFFER_ARB, LOCAL_GL_FALSE);
    A2_(LOCAL_WGL_STEREO_ARB, LOCAL_GL_FALSE);

    A1_(0);

#define MAX_NUM_FORMATS 256
    UINT numFormats = MAX_NUM_FORMATS;
    int formats[MAX_NUM_FORMATS];

    if (!sWGLLibrary.fChoosePixelFormat(gDummyWindowDC,
                                        attribs.Elements(), NULL,
                                        numFormats, formats, &numFormats)
        || numFormats == 0)
    {
        return nsnull;
    }

    
    int chosenFormat = formats[0];

    nsTArray<int> pbattribs;
    pbattribs.AppendElement(LOCAL_WGL_TEXTURE_FORMAT_ARB);
    if (aFormat.alpha > 0) {
        pbattribs.AppendElement(LOCAL_WGL_TEXTURE_RGBA_ARB);
    } else {
        pbattribs.AppendElement(LOCAL_WGL_TEXTURE_RGB_ARB);
    }
    pbattribs.AppendElement(LOCAL_WGL_TEXTURE_TARGET_ARB);
    pbattribs.AppendElement(LOCAL_WGL_TEXTURE_2D_ARB);

    
    
    

    pbattribs.AppendElement(0);

    HANDLE pbuffer = sWGLLibrary.fCreatePbuffer(gDummyWindowDC, chosenFormat,
                                                aSize.width, aSize.height,
                                                pbattribs.Elements());
    if (!pbuffer) {
        return nsnull;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(pbuffer, chosenFormat);
    glContext->Init();

    return glContext.forget().get();
}

already_AddRefed<GLContext>
GLContextProvider::CreateForNativePixmapSurface(gfxASurface *aSurface)
{
    return nsnull;
}

} 
} 
