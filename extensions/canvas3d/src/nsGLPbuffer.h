





































#ifndef NSGLPBUFFER_H_
#define NSGLPBUFFER_H_

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include "glew.h"

#ifdef XP_WIN
#include <windows.h>
#include "wglew.h"
#endif

#if defined(XP_UNIX) && defined(MOZ_X11)
#include "GL/glx.h"
#endif

#ifdef XP_MACOSX
#include <AGL/agl.h>
#endif

class nsCanvasRenderingContextGLPrivate;

class nsGLPbuffer {
public:
    nsGLPbuffer();
    ~nsGLPbuffer();

    PRBool Init(nsCanvasRenderingContextGLPrivate *priv);
    PRBool Resize(PRInt32 width, PRInt32 height);

    void Destroy();

    void MakeContextCurrent();

    void SwapBuffers();
    gfxImageSurface* ThebesSurface();

    inline GLEWContext *glewGetContext() {
        return &mGlewContext;
    }

#ifdef XP_WIN
    inline WGLEWContext *wglewGetContext() {
        return &mWGlewContext;
    }
#endif

    PRInt32 Width() { return mWidth; }
    PRInt32 Height() { return mHeight; }

protected:
    static void *sCurrentContextToken;

    nsCanvasRenderingContextGLPrivate *mPriv;

    PRInt32 mWidth, mHeight;

    GLEWContext mGlewContext;

    nsRefPtr<gfxImageSurface> mThebesSurface;

#ifdef XP_WIN
    
    HWND mGlewWindow;
    HDC mGlewDC;
    HGLRC mGlewWglContext;

    
    HPBUFFERARB mPbuffer;
    HDC mPbufferDC;
    HGLRC mPbufferContext;

    WGLEWContext mWGlewContext;
#endif

#if defined(XP_UNIX) && defined(MOZ_X11)
    Display     *mDisplay;
    GLXFBConfig mFBConfig;
    GLXPbuffer mPbuffer;
    GLXContext mPbufferContext;
#endif

#ifdef XP_MACOSX
    AGLPixelFormat mPixelFormat;
    AGLContext mContext;
    AGLPbuffer mPbuffer;
#endif
};

#endif 
