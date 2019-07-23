






































#include "nsICanvasRenderingContextGL.h"

#include "nsGLPbuffer.h"
#include "nsCanvasRenderingContextGL.h"

#include "gfxContext.h"

void *nsGLPbuffer::sCurrentContextToken = nsnull;

nsGLPbuffer::nsGLPbuffer()
    : mWidth(0), mHeight(0),
#ifdef XP_WIN
    mGlewWindow(nsnull), mGlewDC(nsnull), mGlewWglContext(nsnull),
    mPbufferDC(nsnull), mPbufferContext(nsnull)
#endif
{
}

PRBool
nsGLPbuffer::Init(nsCanvasRenderingContextGLPrivate *priv)
{
    mPriv = priv;
    
#ifdef XP_WIN
    WNDCLASS wc;
    PIXELFORMATDESCRIPTOR pfd;

    if (!GetClassInfo(GetModuleHandle(NULL), "GLEW", &wc)) {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = "GLEW";

        if (!RegisterClass(&wc)) {
            mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: RegisterClass failed"));
            return PR_FALSE;
        }
    }

    
    mGlewWindow = CreateWindow("GLEW", "GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                               CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!mGlewWindow) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: CreateWindow failed"));
        return PR_FALSE;
    }

    
    mGlewDC = GetDC(mGlewWindow);
    if (!mGlewDC) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: GetDC failed"));
        return PR_FALSE;
    }

    
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    int pixelformat = ChoosePixelFormat(mGlewDC, &pfd);

    
    if (!SetPixelFormat(mGlewDC, pixelformat, &pfd)) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: SetPixelFormat failed"));
        return PR_FALSE;
    }

    
    mGlewWglContext = wglCreateContext(mGlewDC);
    if (!mGlewWglContext) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: wglCreateContext failed"));
        return PR_FALSE;
    }

    if (!wglMakeCurrent(mGlewDC, mGlewWglContext)) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: wglMakeCurrent failed"));
        return PR_FALSE;
    }

    if (wglewInit() != GLEW_OK) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: WGLEW init failed"));
        return PR_FALSE;
    }

    fprintf (stderr, "nsGLPbuffer::Init!\n");
#else
    return PR_FALSE;
#endif

    if (glewInit() != GLEW_OK) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: GLEW init failed"));
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsGLPbuffer::Resize(PRInt32 width, PRInt32 height)
{
    if (mWidth == width &&
        mHeight == height)
    {
        return PR_TRUE;
    }

    Destroy();

#ifdef XP_WIN
    if (!wglMakeCurrent(mGlewDC, mGlewWglContext)) {
        fprintf (stderr, "Error: %d\n", GetLastError());
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: wglMakeCurrent failed"));
        return PR_FALSE;
    }

    if (!WGLEW_ARB_pbuffer || !WGLEW_ARB_pixel_format)
    {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: WGL_ARB_pbuffer or WGL_ARB_pixel_format not available."));
        return NS_ERROR_FAILURE;
    }

    int attribs[] = {
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_FALSE,

        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,

        WGL_COLOR_BITS_ARB, 32,
        WGL_RED_BITS_ARB, 8,
        WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8,
        WGL_ALPHA_BITS_ARB, 8,
        0
    };

    float fattribs[] = { 0.0f };
    UINT numFormats = 0;

    

    if (!wglChoosePixelFormatARB(mGlewDC,
                                 attribs,
                                 fattribs,
                                 0,
                                 NULL,
                                 &numFormats) ||
        numFormats == 0)
    {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: wglChoosePixelFormat failed (or couldn't find any matching formats)."));
        ReleaseDC(NULL, mGlewDC);
        return NS_ERROR_FAILURE;
    }

    nsAutoArrayPtr<int> formats = new int [numFormats];
    wglChoosePixelFormatARB(mGlewDC, attribs, NULL, numFormats, formats, &numFormats);

    int chosenFormat = -1;
    int question,answer;

    for (int priority = 6; priority > 0; priority--) {

        

        for (UINT i = 0; i < numFormats; i++) {
            int fmt = formats[i];
#define CHECK_ATTRIB(q, test)                                           \
            question = (q);                                             \
            if (!wglGetPixelFormatAttribivARB(mGlewDC, fmt, 0, 1, &question, &answer)) { \
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
                    CHECK_ATTRIB(WGL_SAMPLE_BUFFERS_ARB, answer != 1)
                case 3:
                    CHECK_ATTRIB(WGL_SAMPLES_ARB, answer != 2)
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
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: Couldn't find a suitable pixel format!"));
        return NS_ERROR_FAILURE;
    }
    
    
    fprintf (stderr, "***** Chose pixel format: %d\n", chosenFormat);
    
    int pbattribs = 0;
    mPbuffer = wglCreatePbufferARB(mGlewDC, chosenFormat, width, height, &pbattribs);
    if (!mPbuffer) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: Failed to create pbuffer"));
        return NS_ERROR_FAILURE;
    }

    mPbufferDC = wglGetPbufferDCARB(mPbuffer);
    mPbufferContext = wglCreateContext(mPbufferDC);

    mThebesSurface = new gfxImageSurface(gfxIntSize(width, height), gfxASurface::ImageFormatARGB32);
#if 0
    if (mThebesSurface->Status() != 0) {
        fprintf (stderr, "image surface failed\n");
        return PR_FALSE;
    }
#endif

    {
        nsRefPtr<gfxContext> ctx = new gfxContext(mThebesSurface);
        ctx->SetColor(gfxRGBA(0, 1, 0, 1));
        ctx->Paint();
    }

#endif

    mWidth = width;
    mHeight = height;

    return PR_TRUE;
}

void
nsGLPbuffer::Destroy()
{
    sCurrentContextToken = nsnull;
    mThebesSurface = nsnull;

#ifdef XP_WIN
    if (mPbuffer) {
        wglDeleteContext(mPbufferContext);
        wglDestroyPbufferARB(mPbuffer);
        mPbuffer = nsnull;
    }
#endif 
}

nsGLPbuffer::~nsGLPbuffer()
{
#ifdef XP_WIN
    if (mGlewWindow) {
        wglDeleteContext(mGlewWglContext);
        DestroyWindow(mGlewWindow);
        mGlewWindow = nsnull;
    }
#endif
}

void
nsGLPbuffer::MakeContextCurrent()
{
#ifdef XP_WIN
    if (sCurrentContextToken == mPbufferContext)
        return;

    wglMakeCurrent (mPbufferDC, mPbufferContext);
    sCurrentContextToken = mPbufferContext;
#endif
}

void
nsGLPbuffer::SwapBuffers()
{
    MakeContextCurrent();
    glReadPixels (0, 0, mWidth, mHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, mThebesSurface->Data());
}

gfxImageSurface*
nsGLPbuffer::ThebesSurface()
{
    return mThebesSurface;
}
