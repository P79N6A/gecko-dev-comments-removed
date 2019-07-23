





































#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"

#include "nsGLPbuffer.h"
#include "WebGLContext.h"

#include <OpenGL/OpenGL.h>

#include "gfxContext.h"

using namespace mozilla;

static PRUint32 gActiveBuffers = 0;

nsGLPbufferCGL::nsGLPbufferCGL()
    : mContext(nsnull), mPbuffer(nsnull), fFlush(nsnull)
{
    gActiveBuffers++;
    fprintf (stderr, "nsGLPbuffer: gActiveBuffers: %d\n", gActiveBuffers);
}

PRBool
nsGLPbufferCGL::Init(WebGLContext *priv)
{
    mPriv = priv;
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
    
    CGLPixelFormatAttribute attrib[] = {
        kCGLPFAAccelerated,
        kCGLPFAMinimumPolicy,
        kCGLPFAPBuffer,
        kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
        kCGLPFAAlphaSize, (CGLPixelFormatAttribute) 8,
        kCGLPFADepthSize, (CGLPixelFormatAttribute) 8,
        (CGLPixelFormatAttribute) 0
    };

#if 0
    if (false && prefAntialiasing > 0) {
        attrib[12] = AGL_SAMPLE_BUFFERS_ARB;
        attrib[13] = 1;

        attrib[14] = AGL_SAMPLES_ARB;
        attrib[15] = 1 << prefAntialiasing;
    }
#endif

    CGLError err;

    GLint npix;
    err = CGLChoosePixelFormat(attrib, &mPixelFormat, &npix);
    if (err) {
        fprintf (stderr, "CGLChoosePixelFormat failed: %d\n", err);
        return PR_FALSE;
    }

    
    Resize(2, 2);
    MakeContextCurrent();

    if (!mGLWrap.OpenLibrary("/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib")) {
        LogMessage("Canvas 3D: Failed to open LibGL.dylib (tried system OpenGL.framework)");
        return PR_FALSE;
    }

    if (!mGLWrap.Init(GLES20Wrap::TRY_NATIVE_GL)) {
        LogMessage("Canvas 3D: GLWrap init failed");
        return PR_FALSE;
    }

    fFlush = (PFNGLFLUSHPROC) mGLWrap.LookupSymbol("glFlush", true);

    return PR_TRUE;
}

PRBool
nsGLPbufferCGL::Resize(PRInt32 width, PRInt32 height)
{
    if (mWidth == width &&
        mHeight == height)
    {
        return PR_TRUE;
    }

    Destroy();

    mThebesSurface = nsnull;
    mQuartzSurface = nsnull;
        
    CGLError err;

    err = CGLCreateContext(mPixelFormat, NULL, &mContext);
    if (err) {
        fprintf (stderr, "CGLCreateContext failed: %d\n", err);
        return PR_FALSE;
    }

    err = CGLCreatePBuffer(width, height, LOCAL_GL_TEXTURE_RECTANGLE_EXT, LOCAL_GL_RGBA, 0, &mPbuffer);
    if (err) {
        fprintf (stderr, "CGLCreatePBuffer failed: %d\n", err);
        return PR_FALSE;
    }

    GLint screen;
    err = CGLGetVirtualScreen(mContext, &screen);
    if (err) {
        fprintf (stderr, "CGLGetVirtualScreen failed: %d\n", err);
        return PR_FALSE;
    }

    err = CGLSetPBuffer(mContext, mPbuffer, 0, 0, screen);
    if (err) {
        fprintf (stderr, "CGLSetPBuffer failed: %d\n", err);
        return PR_FALSE;
    }

    mWidth = width;
    mHeight = height;

    return PR_TRUE;
}

void
nsGLPbufferCGL::Destroy()
{
    sCurrentContextToken = nsnull;
    mThebesSurface = nsnull;

    if (mContext) {
        CGLDestroyContext(mContext);
        mContext = nsnull;
    }
    if (mPbuffer) {
        CGLDestroyPBuffer(mPbuffer);
        mPbuffer = nsnull;
    }
}

nsGLPbufferCGL::~nsGLPbufferCGL()
{
    Destroy();

    if (mPixelFormat) {
        CGLDestroyPixelFormat(mPixelFormat);
        mPixelFormat = nsnull;
    }

    gActiveBuffers--;
    fprintf (stderr, "nsGLPbuffer: gActiveBuffers: %d\n", gActiveBuffers);
    fflush (stderr);
}

void
nsGLPbufferCGL::MakeContextCurrent()
{
    CGLError err = CGLSetCurrentContext (mContext);
    if (err) {
        fprintf (stderr, "CGLSetCurrentContext failed: %d\n", err);
    }
}

void
nsGLPbufferCGL::SwapBuffers()
{
    MakeContextCurrent();

    
    
    if (fFlush)
        fFlush();

    mImageNeedsUpdate = PR_TRUE;
}

gfxASurface*
nsGLPbufferCGL::ThebesSurface()
{
    if (!mThebesSurface) {
        mThebesSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), gfxASurface::ImageFormatARGB32);
        if (mThebesSurface->CairoStatus() != 0) {
            fprintf (stderr, "image surface failed\n");
            return nsnull;
        }

        mQuartzSurface = new gfxQuartzImageSurface(mThebesSurface);

        mImageNeedsUpdate = PR_TRUE;
    }

    if (mImageNeedsUpdate) {
        MakeContextCurrent();
        mGLWrap.fReadPixels (0, 0, mWidth, mHeight, LOCAL_GL_BGRA, LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV, mThebesSurface->Data());

        mQuartzSurface->Flush();

        mImageNeedsUpdate = PR_FALSE;
    }

    return mQuartzSurface;
}
