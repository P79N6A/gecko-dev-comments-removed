









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
    Surface(Display *display, IDirect3DSwapChain9 *swapChain, IDirect3DSurface9* depthStencil, const egl::Config *config);

    ~Surface();

    HWND getWindowHandle();
    bool swap();

    virtual EGLint getWidth() const;
    virtual EGLint getHeight() const;

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();

  private:
    DISALLOW_COPY_AND_ASSIGN(Surface);

    Display *const mDisplay;
    IDirect3DSwapChain9 *const mSwapChain;
    IDirect3DSurface9 *mBackBuffer;
    IDirect3DSurface9 *mRenderTarget;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DTexture9 *mFlipTexture;

    void applyFlipState(IDirect3DDevice9 *device, IDirect3DTexture9 *source);
    void restoreState(IDirect3DDevice9 *device);
    void writeRecordableFlipState(IDirect3DDevice9 *device, IDirect3DTexture9 *source);
    IDirect3DStateBlock9 *mFlipState;
    IDirect3DStateBlock9 *mPreFlipState;
    IDirect3DSurface9 *mPreFlipBackBuffer;
    IDirect3DSurface9 *mPreFlipDepthStencil;

    const egl::Config *mConfig;    
    EGLint mHeight;                
    EGLint mWidth;                 






    EGLint mPixelAspectRatio;      
    EGLenum mRenderBuffer;         
    EGLenum mSwapBehavior;         




};
}

#endif   
