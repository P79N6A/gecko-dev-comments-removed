





































#ifndef NSGLPBUFFER_H_
#define NSGLPBUFFER_H_

#ifdef C3D_STANDALONE_BUILD
#include "c3d-standalone.h"
#endif

#include "nsStringGlue.h"

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#ifdef USE_EGL
typedef int EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
#endif

#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#endif

#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

#if defined(WINCE) && defined(CAIRO_HAS_DDRAW_SURFACE)
#include "gfxDDrawSurface.h"
#endif

#ifdef USE_GLX
#define GLX_GLXEXT_LEGACY
#include "GL/glx.h"
#endif

#ifdef USE_CGL
#include "gfxQuartzImageSurface.h"
#include <OpenGL/CGLTypes.h>
#endif

#include "glwrap.h"

namespace mozilla {
class WebGLContext;
}

class nsGLPbuffer {
public:
    nsGLPbuffer() : mWidth(0), mHeight(0), mPriv(0) { }
    virtual ~nsGLPbuffer() { }

    virtual PRBool Init(mozilla::WebGLContext *priv) = 0;
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
    mozilla::WebGLContext *mPriv;

    void Premultiply(unsigned char *src, unsigned int len);

    void LogMessage (const char *fmt, ...);
};

class nsGLPbufferOSMESA :
    public nsGLPbuffer
{
public:
    nsGLPbufferOSMESA();
    virtual ~nsGLPbufferOSMESA();

    virtual PRBool Init(mozilla::WebGLContext *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

protected:
    nsRefPtr<gfxImageSurface> mThebesSurface;
    PrivateOSMesaContext mMesaContext;
};

#ifdef USE_CGL
class nsGLPbufferCGL :
    public nsGLPbuffer
{
public:
    nsGLPbufferCGL();
    virtual ~nsGLPbufferCGL();

    virtual PRBool Init(mozilla::WebGLContext *priv);
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

#ifdef USE_GLX
class nsGLPbufferGLX :
    public nsGLPbuffer
{
public:
    nsGLPbufferGLX();
    virtual ~nsGLPbufferGLX();

    virtual PRBool Init(mozilla::WebGLContext *priv);
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

#ifdef USE_EGL
class nsGLPbufferEGL :
    public nsGLPbuffer
{
public:
    nsGLPbufferEGL();
    virtual ~nsGLPbufferEGL();

    virtual PRBool Init(mozilla::WebGLContext *priv);
    virtual PRBool Resize(PRInt32 width, PRInt32 height);
    virtual void Destroy();

    virtual void MakeContextCurrent();
    virtual void SwapBuffers();

    virtual gfxASurface* ThebesSurface();

protected:
    EGLDisplay mDisplay;
    EGLConfig mConfig;
    EGLSurface mSurface;
    EGLContext mContext;

#if defined(XP_WIN)
    nsRefPtr<gfxImageSurface> mThebesSurface;
    nsRefPtr<gfxWindowsSurface> mWindowsSurface;
#elif defined(MOZ_X11)
    nsRefPtr<gfxImageSurface> mThebesSurface;
    nsRefPtr<gfxXlibSurface> mXlibSurface;
    Visual *mVisual;
#endif
};
#endif

#ifdef USE_WGL
class nsGLPbufferWGL :
    public nsGLPbuffer
{
public:
    nsGLPbufferWGL();
    virtual ~nsGLPbufferWGL();

    virtual PRBool Init(mozilla::WebGLContext *priv);
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
