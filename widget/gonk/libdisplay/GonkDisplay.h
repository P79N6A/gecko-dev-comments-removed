














#ifndef GONKDISPLAY_H
#define GONKDISPLAY_H

#include <system/window.h>

namespace mozilla {

typedef void * EGLDisplay;
typedef void * EGLSurface;

class GonkDisplay {
public:
    virtual ANativeWindow* GetNativeWindow() = 0;

    virtual void SetEnabled(bool enabled) = 0;

    typedef void (*OnEnabledCallbackType)(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback) = 0;

    virtual void* GetHWCDevice() = 0;

    virtual void* GetFBSurface() = 0;

    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur) = 0;

    virtual ANativeWindowBuffer* DequeueBuffer() = 0;

    virtual bool QueueBuffer(ANativeWindowBuffer* buf) = 0;

    float xdpi;
    uint32_t surfaceformat;
};

__attribute__ ((weak))
GonkDisplay* GetGonkDisplay();

}
#endif 
