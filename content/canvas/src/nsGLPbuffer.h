





































#ifndef NSGLPBUFFER_H_
#define NSGLPBUFFER_H_

#ifdef C3D_STANDALONE_BUILD
#include "c3d-standalone.h"
#endif

#include "nsStringGlue.h"

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#endif

#if defined(XP_UNIX) && defined(MOZ_X11)
#include "GL/glx.h"
#endif

#ifdef XP_MACOSX
#include "gfxQuartzImageSurface.h"
#include <OpenGL/CGLTypes.h>
#endif

#include "glwrap.h"

class nsCanvasRenderingContextGLPrivate;

class nsGLPbuffer {
public:
    nsGLPbuffer() : mWidth(0), mHeight(0), mPriv(0) { }
    virtual ~nsGLPbuffer() { }

    virtual PRBool Init(nsCanvasRenderingContextGLPrivate *priv) = 0;
    virtual PRBool Resize(PRInt32 width, PRInt32 height) = 0;
    virtual void Destroy() = 0;

    virtual void MakeContextCurrent() = 0;
    virtual void SwapBuffers() = 0;

    virtual gfxASurface* ThebesSurface() = 0;

    PRInt32 Width() { return mWidth; }
    PRInt32 Height() { return mHeight; }

    GLES20Wrap *GL() { return &mGLWrap; }

protected:
    PRInt32 mWidth, mHeight;

    GLES20Wrap mGLWrap;

    static void *sCurrentContextToken;
    nsCanvasRenderingContextGLPrivate *mPriv;

    void Premultiply(unsigned char *src, unsigned int len);

    void LogMessage (const nsCString& errorString);
    void LogMessagef (const char *fmt, ...);
};

class nsGLPbufferOSMESA :
    public nsGLPbuffer
{
public:
    nsGLPbufferOSMESA();
    virtual ~nsGLPbufferOSMESA();

    virtual PRBool Init(nsCanvasRenderingContextGLPrivate *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

protected:
    nsRefPtr<gfxImageSurface> mThebesSurface;
    PrivateOSMesaContext mMesaContext;
};

#ifdef XP_MACOSX
class nsGLPbufferCGL :
    public nsGLPbuffer
{
public:
    nsGLPbufferCGL();
    virtual ~nsGLPbufferCGL();

    virtual PRBool Init(nsCanvasRenderingContextGLPrivate *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

    CGLPixelFormatObj GetCGLPixelFormat() { return mPixelFormat; }
    CGLContextObj GetCGLContext() { return mContext; }
    CGLPBufferObj GetCGLPbuffer() { return mPbuffer; }

protected:
    CGLPixelFormatObj mPixelFormat;
    CGLContextObj mContext;
    CGLPBufferObj mPbuffer;

    PRBool mImageNeedsUpdate;
    nsRefPtr<gfxImageSurface> mThebesSurface;
    nsRefPtr<gfxQuartzImageSurface> mQuartzSurface;

    typedef void (GLAPIENTRY * PFNGLFLUSHPROC) (void);
    PFNGLFLUSHPROC fFlush;
};
#endif

#if defined(XP_UNIX) && defined(MOZ_X11)
class nsGLPbufferGLX :
    public nsGLPbuffer
{
public:
    nsGLPbufferGLX();
    virtual ~nsGLPbufferGLX();

    virtual PRBool Init(nsCanvasRenderingContextGLPrivate *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

protected:
    nsRefPtr<gfxImageSurface> mThebesSurface;

    Display     *mDisplay;
    GLXFBConfig mFBConfig;
    GLXPbuffer mPbuffer;
    GLXContext mPbufferContext;
};
#endif

#ifdef XP_WIN
class nsGLPbufferWGL :
    public nsGLPbuffer
{
public:
    nsGLPbufferWGL();
    virtual ~nsGLPbufferWGL();

    virtual PRBool Init(nsCanvasRenderingContextGLPrivate *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

protected:
    
    HWND mGlewWindow;
    HDC mGlewDC;
    HANDLE mGlewWglContext;

    
    HANDLE mPbuffer;
    HDC mPbufferDC;
    HANDLE mPbufferContext;

    nsRefPtr<gfxImageSurface> mThebesSurface;
    nsRefPtr<gfxWindowsSurface> mWindowsSurface;
};
#endif


#endif 
