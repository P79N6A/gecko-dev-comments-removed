








#ifndef LIBGLESV2_RENDERER_SWAPCHAIN_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN_H_

#include "common/angleutils.h"
#include "common/NativeWindow.h"
#include "common/platform.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#if !defined(ANGLE_FORCE_VSYNC_OFF)
#define ANGLE_FORCE_VSYNC_OFF 0
#endif

namespace rx
{

class SwapChain
{
  public:
    SwapChain(rx::NativeWindow nativeWindow, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
        : mNativeWindow(nativeWindow), mShareHandle(shareHandle), mBackBufferFormat(backBufferFormat), mDepthBufferFormat(depthBufferFormat)
    {
    }

    virtual ~SwapChain() {};

    virtual EGLint resize(EGLint backbufferWidth, EGLint backbufferSize) = 0;
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval) = 0;
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height) = 0;
    virtual void recreate() = 0;

    virtual HANDLE getShareHandle() {return mShareHandle;};
    virtual void* getKeyedMutex() {return NULL;};

  protected:
    rx::NativeWindow mNativeWindow;  
    const GLenum mBackBufferFormat;
    const GLenum mDepthBufferFormat;

    HANDLE mShareHandle;
};

}
#endif 
