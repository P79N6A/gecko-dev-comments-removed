









#ifndef INCLUDE_SURFACE_H_
#define INCLUDE_SURFACE_H_

#define EGLAPI
#include <EGL/egl.h>
#include <d3d9.h>

#include "common/angleutils.h"

namespace egl
{
class Display;
class Config;

class Surface
{
  public:
    Surface(Display *display, const egl::Config *config, HWND window);
    Surface(Display *display, const egl::Config *config, EGLint width, EGLint height);

    ~Surface();

    void release();
    void resetSwapChain();

    HWND getWindowHandle();
    bool swap();

    virtual EGLint getWidth() const;
    virtual EGLint getHeight() const;

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();

    HANDLE getShareHandle() { return mShareHandle; }

    void setSwapInterval(EGLint interval);
    bool checkForOutOfDateSwapChain();   

private:
    DISALLOW_COPY_AND_ASSIGN(Surface);

    Display *const mDisplay;
    IDirect3DSwapChain9 *mSwapChain;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DSurface9* mRenderTarget;
    IDirect3DTexture9* mOffscreenTexture;

    HANDLE mShareHandle;

    void subclassWindow();
    void unsubclassWindow();
    void resetSwapChain(int backbufferWidth, int backbufferHeight);
    static DWORD convertInterval(EGLint interval);

    const HWND mWindow;            
    bool mWindowSubclassed;        
    const egl::Config *mConfig;    
    EGLint mHeight;                
    EGLint mWidth;                 






    EGLint mPixelAspectRatio;      
    EGLenum mRenderBuffer;         
    EGLenum mSwapBehavior;         




    EGLint mSwapInterval;
    DWORD mPresentInterval;
    bool mPresentIntervalDirty;
};
}

#endif   
