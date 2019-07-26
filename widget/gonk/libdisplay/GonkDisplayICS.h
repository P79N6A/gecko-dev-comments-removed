














#ifndef GONKDISPLAYICS_H
#define GONKDISPLAYICS_H

#include <string.h>

#include "GonkDisplay.h"
#include "ui/FramebufferNativeWindow.h"
#include "hardware/hwcomposer.h"
#include "utils/RefBase.h"

namespace mozilla {

class MOZ_EXPORT GonkDisplayICS : public GonkDisplay {
public:
    GonkDisplayICS();
    ~GonkDisplayICS();

    virtual ANativeWindow* GetNativeWindow();

    virtual void SetEnabled(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback);

    virtual void* GetHWCDevice();

    virtual void* GetFBSurface();

    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur);

    virtual ANativeWindowBuffer* DequeueBuffer();

    virtual bool QueueBuffer(ANativeWindowBuffer* handle);

    virtual void UpdateFBSurface(EGLDisplay dpy, EGLSurface sur);

    virtual void SetFBReleaseFd(int fd);

    virtual int GetPrevFBAcquireFd()
    {
        return -1;
    }

private:
    hw_module_t const*        mModule;
    hwc_composer_device_t*    mHwc;
    android::sp<android::FramebufferNativeWindow> mFBSurface;
};

}

#endif 
