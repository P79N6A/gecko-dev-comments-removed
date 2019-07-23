






































#include "nsICanvasRenderingContextGL.h"

#include "nsGLPbuffer.h"
#include "nsCanvasRenderingContextGL.h"

#include "gfxContext.h"

void *nsGLPbuffer::sCurrentContextToken = nsnull;

static PRUint32 gActiveBuffers = 0;

nsGLPbuffer::nsGLPbuffer()
    : mWidth(0), mHeight(0)
#ifdef XP_WIN
    , mGlewWindow(nsnull), mGlewDC(nsnull), mGlewWglContext(nsnull),
    mPbuffer(nsnull), mPbufferDC(nsnull), mPbufferContext(nsnull)
#elif defined(XP_UNIX) && defined(MOZ_X11)
    , mDisplay(nsnull), mFBConfig(0), mPbuffer(0), mPbufferContext(0)
#elif defined(XP_MACOSX)
    , mContext(nsnull), mPbuffer(nsnull)
#endif
{
    gActiveBuffers++;
    fprintf (stderr, "nsGLPbuffer: gActiveBuffers: %d\n", gActiveBuffers);
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

    PRInt64 t1 = PR_Now();

    if (wglewInit() != GLEW_OK) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: WGLEW init failed"));
        return PR_FALSE;
    }

    PRInt64 t2 = PR_Now();

    fprintf (stderr, "nsGLPbuffer::Init!\n");
#elif defined(XP_UNIX) && defined(MOZ_X11)
    mDisplay = XOpenDisplay(NULL);
    if (!mDisplay) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: XOpenDisplay failed"));
        return PR_FALSE;
    }

    int attrib[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
                     GLX_RENDER_TYPE,   GLX_RGBA_BIT,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DEPTH_SIZE, 1,
                     None };

    int num;
    GLXFBConfig *configs = glXChooseFBConfig(mDisplay, DefaultScreen(mDisplay),
                                             attrib, &num);
    fprintf(stderr, "CANVAS3D FBCONFIG: %d %p\n", num, configs);
    if (!configs) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: No GLXFBConfig found"));
        return PR_FALSE;
    }

    
    mFBConfig = *configs;

    XFree(configs);

    

    mPbufferContext = glXCreateNewContext(mDisplay, mFBConfig, GLX_RGBA_TYPE,
                                          nsnull, True);

    PRInt64 t1 = PR_Now();

    Resize(2, 2);
    MakeContextCurrent();

    PRInt64 t2 = PR_Now();

    fprintf (stderr, "nsGLPbuffer::Init!\n");

#elif defined(XP_MACOSX)
    PRInt64 t1 = PR_Now();
    PRInt64 t2 = t1;

    GLint attrib[] = {
        AGL_RGBA,
        AGL_PBUFFER,
        AGL_RED_SIZE, 8,
        AGL_GREEN_SIZE, 8,
        AGL_BLUE_SIZE, 8,
        AGL_ALPHA_SIZE, 8,
        AGL_DEPTH_SIZE, 1,
        0
    };

    mPixelFormat = aglChoosePixelFormat(NULL, 0, &attrib[0]);
    if (!mPixelFormat)
        return PR_FALSE;

    
    Resize(2, 2);
    MakeContextCurrent();
#else
    return PR_FALSE;
#endif

    if (glewInit() != GLEW_OK) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: GLEW init failed"));
        return PR_FALSE;
    }

    PRInt64 t3 = PR_Now();

    fprintf (stderr, "nsGLPbuffer:: Initialization took t2-t1: %f t3-t2: %f\n",
             ((double)(t2-t1))/1000.0, ((double)(t3-t2))/1000.0);
    fflush (stderr);

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

    mThebesSurface = CanvasGLThebes::CreateImageSurface(gfxIntSize(width, height), gfxASurface::ImageFormatARGB32);
    if (mThebesSurface->CairoStatus() != 0) {
        fprintf (stderr, "image surface failed\n");
        return PR_FALSE;
    }

    
    memset (mThebesSurface->Data(),
            0,
            height * mThebesSurface->Stride());

#ifdef XP_WIN
    if (!wglMakeCurrent(mGlewDC, mGlewWglContext)) {
        fprintf (stderr, "Error: %d\n", GetLastError());
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: wglMakeCurrent failed"));
        return PR_FALSE;
    }

    if (!WGLEW_ARB_pbuffer || !WGLEW_ARB_pixel_format)
    {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: WGL_ARB_pbuffer or WGL_ARB_pixel_format not available."));
        return PR_FALSE;
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
        return PR_FALSE;
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
        return PR_FALSE;
    }
    
    
    fprintf (stderr, "***** Chose pixel format: %d\n", chosenFormat);
    
    int pbattribs = 0;
    mPbuffer = wglCreatePbufferARB(mGlewDC, chosenFormat, width, height, &pbattribs);
    if (!mPbuffer) {
        mPriv->LogMessage(NS_LITERAL_CSTRING("Canvas 3D: Failed to create pbuffer"));
        return PR_FALSE;
    }

    mPbufferDC = wglGetPbufferDCARB(mPbuffer);
    mPbufferContext = wglCreateContext(mPbufferDC);
#elif defined(XP_UNIX) && defined(MOZ_X11)
    int attrib[] = { GLX_PBUFFER_WIDTH, width,
                     GLX_PBUFFER_HEIGHT, height,
                     None };

    mPbuffer = glXCreatePbuffer(mDisplay, mFBConfig, attrib);
#elif defined(XP_MACOSX)
    mContext = aglCreateContext(mPixelFormat, NULL);
    if (!mContext)
        return PR_FALSE;

    GLint screen = aglGetVirtualScreen(mContext);

    if (screen == -1
        || !aglCreatePBuffer(width, height, GL_TEXTURE_RECTANGLE_EXT,
                             GL_RGBA, 0, &mPbuffer)
        || !aglSetPBuffer (mContext, mPbuffer, 0, 0, screen))
    {
        return PR_FALSE;
    }

    MakeContextCurrent();
#endif

    mWidth = width;
    mHeight = height;

    fprintf (stderr, "Resize: %d %d\n", width, height);
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
#elif defined(XP_UNIX) && defined(MOZ_X11)
    if (mPbuffer) {
        glXDestroyPbuffer(mDisplay, mPbuffer);
        mPbuffer = nsnull;
    }
#else defined(XP_MACOSX)
    if (mContext) {
        aglDestroyContext(mContext);
        mContext = nsnull;
    }
    if (mPbuffer) {
        aglDestroyPBuffer(mPbuffer);
        mPbuffer = nsnull;
    }
#endif
}

nsGLPbuffer::~nsGLPbuffer()
{
    Destroy();

#ifdef XP_WIN
    if (mGlewWindow) {
        wglDeleteContext(mGlewWglContext);
        DestroyWindow(mGlewWindow);
        mGlewWindow = nsnull;
    }
#elif defined(XP_UNIX) && defined(MOZ_X11)
    if (mPbuffer)
        glXDestroyPbuffer(mDisplay, mPbuffer);
    if (mPbufferContext)
        glXDestroyContext(mDisplay, mPbufferContext);
    if (mDisplay)
        XCloseDisplay(mDisplay);
#else defined(XP_MACOSX)
    if (mPbuffer)
        aglDestroyPBuffer(mPbuffer);
    if (mContext)
        aglDestroyContext(mContext);
#endif

    gActiveBuffers--;
    fprintf (stderr, "nsGLPbuffer: gActiveBuffers: %d\n", gActiveBuffers);
    fflush (stderr);
}

void
nsGLPbuffer::MakeContextCurrent()
{
#ifdef XP_WIN
    if (sCurrentContextToken == mPbufferContext)
        return;

    wglMakeCurrent (mPbufferDC, mPbufferContext);
    sCurrentContextToken = mPbufferContext;
#elif defined(XP_UNIX) && defined(MOZ_X11)
    glXMakeContextCurrent(mDisplay, mPbuffer, mPbuffer, mPbufferContext);
#elif defined(XP_MACOSX)
    aglSetCurrentContext(mContext);
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
