









#ifndef LIBEGL_SURFACE_H_
#define LIBEGL_SURFACE_H_

#include <EGL/egl.h>

#include "common/angleutils.h"

namespace gl
{
class Texture2D;
}
namespace rx
{
class Renderer;
class SwapChain;
}

namespace egl
{
class Display;
class Config;

class Surface
{
  public:
    Surface(Display *display, const egl::Config *config, HWND window, EGLint fixedSize, EGLint width, EGLint height, EGLint postSubBufferSupported);
    Surface(Display *display, const egl::Config *config, HANDLE shareHandle, EGLint width, EGLint height, EGLenum textureFormat, EGLenum textureTarget);

    virtual ~Surface();

    bool initialize();
    void release();
    bool resetSwapChain();

    HWND getWindowHandle();
    bool swap();
    bool postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height);

    virtual EGLint isPostSubBufferSupported() const;

    virtual rx::SwapChain *getSwapChain() const;

    void setSwapInterval(EGLint interval);
    bool checkForOutOfDateSwapChain();   

    virtual EGLint getConfigID() const;
    virtual EGLint getWidth() const;
    virtual EGLint getHeight() const;
    virtual EGLint getPixelAspectRatio() const;
    virtual EGLenum getRenderBuffer() const;
    virtual EGLenum getSwapBehavior() const;
    virtual EGLenum getTextureFormat() const;
    virtual EGLenum getTextureTarget() const;
    virtual EGLenum getFormat() const;

    virtual void setBoundTexture(gl::Texture2D *texture);
    virtual gl::Texture2D *getBoundTexture() const;

    EGLint isFixedSize() const;

private:
    DISALLOW_COPY_AND_ASSIGN(Surface);

    Display *const mDisplay;
    rx::Renderer *mRenderer;

    HANDLE mShareHandle;
    rx::SwapChain *mSwapChain;

    void subclassWindow();
    void unsubclassWindow();
    bool resizeSwapChain(int backbufferWidth, int backbufferHeight);
    bool resetSwapChain(int backbufferWidth, int backbufferHeight);
    bool swapRect(EGLint x, EGLint y, EGLint width, EGLint height);

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
    EGLint mPostSubBufferSupported;
    EGLint mFixedSize;

    bool mSwapIntervalDirty;
    gl::Texture2D *mTexture;
};
}

#endif   
