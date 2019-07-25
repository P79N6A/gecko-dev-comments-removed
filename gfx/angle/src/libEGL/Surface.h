









#ifndef INCLUDE_SURFACE_H_
#define INCLUDE_SURFACE_H_

#define EGLAPI
#include <EGL/egl.h>
#include <d3d9.h>

#include "common/angleutils.h"

namespace gl
{
class Texture2D;
}

namespace egl
{
class Display;
class Config;

class Surface
{
  public:
    Surface(Display *display, const egl::Config *config, HWND window);
    Surface(Display *display, const egl::Config *config, HANDLE shareHandle, EGLint width, EGLint height, EGLenum textureFormat, EGLenum textureTarget);

    ~Surface();

    bool initialize();
    void release();
    bool resetSwapChain();

    HWND getWindowHandle();
    bool swap();

    virtual EGLint getWidth() const;
    virtual EGLint getHeight() const;

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();
    virtual IDirect3DTexture9 *getOffscreenTexture();

    HANDLE getShareHandle() { return mShareHandle; }

    void setSwapInterval(EGLint interval);
    bool checkForOutOfDateSwapChain();   

    virtual EGLenum getTextureFormat() const;
    virtual EGLenum getTextureTarget() const;
    virtual D3DFORMAT getFormat() const;

    virtual void setBoundTexture(gl::Texture2D *texture);
    virtual gl::Texture2D *getBoundTexture() const;

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
    bool resetSwapChain(int backbufferWidth, int backbufferHeight);
    static DWORD convertInterval(EGLint interval);

    const HWND mWindow;            
    bool mWindowSubclassed;        
    const egl::Config *mConfig;    
    EGLint mHeight;                
    EGLint mWidth;                 






    EGLint mPixelAspectRatio;      
    EGLenum mRenderBuffer;         
    EGLenum mSwapBehavior;         
    EGLenum mTextureFormat;        
    EGLenum mTextureTarget;        


    EGLint mSwapInterval;
    DWORD mPresentInterval;
    bool mPresentIntervalDirty;
    gl::Texture2D *mTexture;
};
}

#endif   
