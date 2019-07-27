














#ifndef GONKDISPLAY_H
#define GONKDISPLAY_H

#include <system/window.h>
#include "mozilla/Types.h"

namespace mozilla {

typedef void * EGLDisplay;
typedef void * EGLSurface;

class MOZ_EXPORT GonkDisplay {
public:
    virtual ANativeWindow* GetNativeWindow() = 0;

    virtual void SetEnabled(bool enabled) = 0;

    typedef void (*OnEnabledCallbackType)(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback) = 0;

    virtual void* GetHWCDevice() = 0;

    virtual void* GetDispSurface() = 0;

    


    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur) = 0;

    virtual ANativeWindowBuffer* DequeueBuffer() = 0;

    virtual bool QueueBuffer(ANativeWindowBuffer* buf) = 0;

    virtual void UpdateDispSurface(EGLDisplay dpy, EGLSurface sur) = 0;

    




    virtual void SetDispReleaseFd(int fd) = 0;

    



    virtual int GetPrevDispAcquireFd() = 0;

    float xdpi;
    int32_t surfaceformat;
};

MOZ_EXPORT __attribute__ ((weak))
GonkDisplay* GetGonkDisplay();

}
#endif 
