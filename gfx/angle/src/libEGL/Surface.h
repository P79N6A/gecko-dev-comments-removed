









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

    ~Surface();

    void release();
    void resetSwapChain();

    HWND getWindowHandle();
    bool swap();

    virtual EGLint getWidth() const;
    virtual EGLint getHeight() const;

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();

    void setSwapInterval(EGLint interval);

  private:
    DISALLOW_COPY_AND_ASSIGN(Surface);

    Display *const mDisplay;
    IDirect3DSwapChain9 *mSwapChain;
    IDirect3DSurface9 *mBackBuffer;
    IDirect3DSurface9 *mRenderTarget;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DTexture9 *mFlipTexture;

    bool checkForOutOfDateSwapChain();

    static DWORD convertInterval(EGLint interval);

    void applyFlipState(IDirect3DDevice9 *device);
    void restoreState(IDirect3DDevice9 *device);
    void writeRecordableFlipState(IDirect3DDevice9 *device);
    void releaseRecordedState(IDirect3DDevice9 *device);
    IDirect3DStateBlock9 *mFlipState;
    IDirect3DStateBlock9 *mPreFlipState;
    IDirect3DSurface9 *mPreFlipBackBuffer;
    IDirect3DSurface9 *mPreFlipDepthStencil;

    const HWND mWindow;            
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
