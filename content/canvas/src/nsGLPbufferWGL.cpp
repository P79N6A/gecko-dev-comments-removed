





































#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"

#include "WebGLContext.h"
#include "nsGLPbuffer.h"

#include "gfxContext.h"
#include "gfxImageSurface.h"

using namespace mozilla;

static PRUint32 gActiveBuffers = 0;

class WGLWrap
    : public LibrarySymbolLoader
{
public:
    WGLWrap() : fCreatePbuffer(0) { }

    bool InitEarly();
    bool Init();

public:
    
    typedef HANDLE (WINAPI * PFNWGLCREATECONTEXTPROC) (HDC hDC);
    PFNWGLCREATECONTEXTPROC fCreateContext;
    typedef BOOL (WINAPI * PFNWGLMAKECURRENTPROC) (HDC hDC, HANDLE hglrc);
    PFNWGLMAKECURRENTPROC fMakeCurrent;
    typedef PROC (WINAPI * PFNWGLGETPROCADDRESSPROC) (LPCSTR proc);
    PFNWGLGETPROCADDRESSPROC fGetProcAddress;
    typedef BOOL (WINAPI * PFNWGLDELETECONTEXTPROC) (HANDLE hglrc);
    PFNWGLDELETECONTEXTPROC fDeleteContext;

    typedef HANDLE (WINAPI * PFNWGLCREATEPBUFFERPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
    PFNWGLCREATEPBUFFERPROC fCreatePbuffer;
    typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFERPROC) (HANDLE hPbuffer);
    PFNWGLDESTROYPBUFFERPROC fDestroyPbuffer;
    typedef HDC (WINAPI * PFNWGLGETPBUFFERDCPROC) (HANDLE hPbuffer);
    PFNWGLGETPBUFFERDCPROC fGetPbufferDC;

    typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATPROC) (HDC hdc, const int* piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
    PFNWGLCHOOSEPIXELFORMATPROC fChoosePixelFormat;
    typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int* piAttributes, int *piValues);
    PFNWGLGETPIXELFORMATATTRIBIVPROC fGetPixelFormatAttribiv;
};

bool
WGLWrap::InitEarly()
{
    if (fCreateContext)
        return true;

    SymLoadStruct symbols[] = {
        { (PRFuncPtr*) &fCreateContext, { "wglCreateContext", NULL } },
        { (PRFuncPtr*) &fMakeCurrent, { "wglMakeCurrent", NULL } },
        { (PRFuncPtr*) &fGetProcAddress, { "wglGetProcAddress", NULL } },
        { (PRFuncPtr*) &fDeleteContext, { "wglDeleteContext", NULL } },
        { NULL, { NULL } }
    };

    return LoadSymbols(&symbols[0], false);
}

bool
WGLWrap::Init()
{
    if (fCreatePbuffer)
        return true;

    SymLoadStruct symbols[] = {
        { (PRFuncPtr*) &fCreatePbuffer, { "wglCreatePbufferARB", "wglCreatePbufferEXT", NULL } },
        { (PRFuncPtr*) &fDestroyPbuffer, { "wglDestroyPbufferARB", "wglDestroyPbufferEXT", NULL } },
        { (PRFuncPtr*) &fGetPbufferDC, { "wglGetPbufferDCARB", "wglGetPbufferDCEXT", NULL } },
        { (PRFuncPtr*) &fChoosePixelFormat, { "wglChoosePixelFormatARB", "wglChoosePixelFormatEXT", NULL } },
        { (PRFuncPtr*) &fGetPixelFormatAttribiv, { "wglGetPixelFormatAttribivARB", "wglGetPixelFormatAttribivEXT", NULL } },
        { NULL, { NULL } }
    };

    return LoadSymbols(&symbols[0], true);
}

static WGLWrap gWGLWrap;

nsGLPbufferWGL::nsGLPbufferWGL()
    : mGlewWindow(nsnull), mGlewDC(nsnull), mGlewWglContext(nsnull),
      mPbuffer(nsnull), mPbufferDC(nsnull), mPbufferContext(nsnull)
{
    gActiveBuffers++;
    fprintf (stderr, "nsGLPbufferWGL: gActiveBuffers: %d\n", gActiveBuffers);
}

PRBool
nsGLPbufferWGL::Init(WebGLContext *priv)
{
    
    char *opengl32 = "C:\\WINDOWS\\SYSTEM32\\OPENGL32.DLL";

    if (!gWGLWrap.OpenLibrary(opengl32))
        return PR_FALSE;

    if (!gWGLWrap.InitEarly())
        return PR_FALSE;

    gWGLWrap.SetLookupFunc((LibrarySymbolLoader::PlatformLookupFunction) gWGLWrap.fGetProcAddress);

    mPriv = priv;
    
    WNDCLASSW wc;
    PIXELFORMATDESCRIPTOR pfd;

    if (!GetClassInfoW(GetModuleHandle(NULL), L"GLEW", &wc)) {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = L"GLEW";

        if (!RegisterClassW(&wc)) {
            LogMessage("Canvas 3D: RegisterClass failed");
            return PR_FALSE;
        }
    }

    
    mGlewWindow = CreateWindowW(L"GLEW", L"GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!mGlewWindow) {
        LogMessage("Canvas 3D: CreateWindow failed");
        return PR_FALSE;
    }

    
    mGlewDC = GetDC(mGlewWindow);
    if (!mGlewDC) {
        LogMessage("Canvas 3D: GetDC failed");
        return PR_FALSE;
    }

    
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    int pixelformat = ChoosePixelFormat(mGlewDC, &pfd);

    
    if (!SetPixelFormat(mGlewDC, pixelformat, &pfd)) {
        LogMessage("Canvas 3D: SetPixelFormat failed");
        return PR_FALSE;
    }

    
    mGlewWglContext = gWGLWrap.fCreateContext(mGlewDC);
    if (!mGlewWglContext) {
        LogMessage("Canvas 3D: wglCreateContext failed");
        return PR_FALSE;
    }

    if (!gWGLWrap.fMakeCurrent(mGlewDC, (HGLRC) mGlewWglContext)) {
        LogMessage("Canvas 3D: wglMakeCurrent failed");
        return PR_FALSE;
    }

    
    
    if (!gWGLWrap.Init())
        return PR_FALSE;

    
    if (!mGLWrap.OpenLibrary(opengl32)) {
        LogMessage("Canvas 3D: Failed to open opengl32.dll (only looked in c:\\windows\\system32, fixme)");
        return PR_FALSE;
    }

    mGLWrap.SetLookupFunc((LibrarySymbolLoader::PlatformLookupFunction) gWGLWrap.fGetProcAddress);

    if (!mGLWrap.Init(GLES20Wrap::TRY_NATIVE_GL)) {
        LogMessage("Canvas 3D: GLWrap init failed");
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsGLPbufferWGL::Resize(PRInt32 width, PRInt32 height)
{
    if (mWidth == width &&
        mHeight == height)
    {
        return PR_TRUE;
    }

    Destroy();

    nsresult rv;

    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetBranch("extensions.canvas3d.", getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    PRInt32 prefAntialiasing;
    rv = prefBranch->GetIntPref("antialiasing", &prefAntialiasing);
    if (NS_FAILED(rv))
        prefAntialiasing = 0;

    mThebesSurface = new gfxImageSurface(gfxIntSize(width, height), gfxASurface::ImageFormatARGB32);
    if (mThebesSurface->CairoStatus() != 0) {
        fprintf (stderr, "image surface failed\n");
        return PR_FALSE;
    }

    
    memset (mThebesSurface->Data(),
            0,
            height * mThebesSurface->Stride());

    if (!gWGLWrap.fMakeCurrent(mGlewDC, (HGLRC) mGlewWglContext)) {
        fprintf (stderr, "Error: %d\n", GetLastError());
        LogMessage("Canvas 3D: wglMakeCurrent failed");
        return PR_FALSE;
    }

    PRBool ignoreAA = PR_FALSE;
    int attribs[] = {
        WGL_SUPPORT_OPENGL_ARB, LOCAL_GL_TRUE,
        WGL_DRAW_TO_PBUFFER_ARB, LOCAL_GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, LOCAL_GL_FALSE,

        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,

        WGL_COLOR_BITS_ARB, 32,
        WGL_RED_BITS_ARB, 8,
        WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8,
        WGL_ALPHA_BITS_ARB, 8,

        0, 0,
        0, 0,
        0
    };

    float fattribs[] = { 0.0f };

    
    
    
#define MAX_NUM_FORMATS 256
    UINT numFormats = MAX_NUM_FORMATS;
    nsAutoArrayPtr<int> formats = new int[numFormats];

    

TRY_FIND_AGAIN:
    if (ignoreAA) {
        attribs[18] = 0;
    } else if (prefAntialiasing > 0) {
        attribs[18] = WGL_SAMPLE_BUFFERS_ARB;
        attribs[19] = 1;
        attribs[20] = WGL_SAMPLES_ARB;
        attribs[21] = 1 << prefAntialiasing;
    }

    if (!gWGLWrap.fChoosePixelFormat(mGlewDC,
                                     attribs,
                                     NULL,
                                     numFormats,
                                     formats,
                                     &numFormats) ||
        numFormats == 0)
    {
        if (!ignoreAA) {
            ignoreAA = PR_TRUE;
            goto TRY_FIND_AGAIN;
        }

        LogMessage("Canvas 3D: wglChoosePixelFormat failed (or couldn't find any matching formats).");
        ReleaseDC(NULL, mGlewDC);
        return PR_FALSE;
    }

    int chosenFormat = -1;
    int question,answer;

    for (int priority = 6; priority > 0; priority--) {

        

        for (UINT i = 0; i < numFormats; i++) {
            int fmt = formats[i];
#define CHECK_ATTRIB(q, test)                                           \
            question = (q);                                             \
            if (!gWGLWrap.fGetPixelFormatAttribiv(mGlewDC, fmt, 0, 1, &question, &answer)) { \
                /*fprintf (stderr, "check for %d failed\n", q);*/       \
                continue;                                               \
            }                                                           \
            /*fprintf (stderr, #q " -> %d\n", answer);*/                \
            if (test) {                                                 \
                continue;                                               \
            }

            
            switch (priority) {
                case 6:
                    CHECK_ATTRIB(WGL_ACCUM_BITS_ARB, answer != 0)
                case 5:
                    CHECK_ATTRIB(WGL_STENCIL_BITS_ARB, answer != 0)
                
                case 4:
                    CHECK_ATTRIB(WGL_SAMPLE_BUFFERS_ARB, answer != (prefAntialiasing != 0))
                case 3:
                    CHECK_ATTRIB(WGL_SAMPLES_ARB, answer != (prefAntialiasing ? (1 << prefAntialiasing) : 0))
                case 2:
                    CHECK_ATTRIB(WGL_DEPTH_BITS_ARB, answer < 8)
                case 1:
                    CHECK_ATTRIB(WGL_COLOR_BITS_ARB, answer != 32)
                default:
                    chosenFormat = fmt;
            }

#undef CHECK_ATTRIB
        }

        if (chosenFormat != -1)
            break;
    }

    if (chosenFormat == -1) {
        LogMessage("Canvas 3D: Couldn't find a suitable pixel format!");
        return PR_FALSE;
    }
    
    
    fprintf (stderr, "***** Chose pixel format: %d\n", chosenFormat);
    
    int pbattribs = 0;
    mPbuffer = gWGLWrap.fCreatePbuffer(mGlewDC, chosenFormat, width, height, &pbattribs);
    if (!mPbuffer) {
        LogMessage("Canvas 3D: Failed to create pbuffer");
        return PR_FALSE;
    }

    mPbufferDC = gWGLWrap.fGetPbufferDC(mPbuffer);
    mPbufferContext = gWGLWrap.fCreateContext(mPbufferDC);

    mWindowsSurface = new gfxWindowsSurface(gfxIntSize(width, height), gfxASurface::ImageFormatARGB32);
    if (mWindowsSurface && mWindowsSurface->CairoStatus() == 0)
        mThebesSurface = mWindowsSurface->GetImageSurface();

    mWidth = width;
    mHeight = height;

    fprintf (stderr, "Resize: %d %d\n", width, height);
    return PR_TRUE;
}

void
nsGLPbufferWGL::Destroy()
{
    sCurrentContextToken = nsnull;
    mThebesSurface = nsnull;

    if (mPbuffer) {
        gWGLWrap.fDeleteContext((HGLRC) mPbufferContext);
        gWGLWrap.fDestroyPbuffer(mPbuffer);
        mPbuffer = nsnull;
    }
}

nsGLPbufferWGL::~nsGLPbufferWGL()
{
    Destroy();

    if (mGlewWglContext) {
        gWGLWrap.fDeleteContext((HGLRC) mGlewWglContext);
        mGlewWglContext = nsnull;
    }

    if (mGlewWindow) {
        DestroyWindow(mGlewWindow);
        mGlewWindow = nsnull;
    }

    gActiveBuffers--;
    fprintf (stderr, "nsGLPbufferWGL: gActiveBuffers: %d\n", gActiveBuffers);
    fflush (stderr);
}

void
nsGLPbufferWGL::MakeContextCurrent()
{
    if (sCurrentContextToken == mPbufferContext)
        return;

    gWGLWrap.fMakeCurrent (mPbufferDC, (HGLRC) mPbufferContext);
    sCurrentContextToken = mPbufferContext;
}

void
nsGLPbufferWGL::SwapBuffers()
{
    MakeContextCurrent();
    mGLWrap.fReadPixels (0, 0, mWidth, mHeight, LOCAL_GL_BGRA, LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV, mThebesSurface->Data());

    
    int len = mWidth*mHeight*4;
    unsigned char *src = mThebesSurface->Data();
    Premultiply(src, len);
}

gfxASurface*
nsGLPbufferWGL::ThebesSurface()
{
    return mThebesSurface;
}
