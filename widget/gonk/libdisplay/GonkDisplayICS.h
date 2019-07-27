














#ifndef GONKDISPLAYICS_H
#define GONKDISPLAYICS_H

#include <string.h>

#include "GonkDisplay.h"
#include "ui/FramebufferNativeWindow.h"
#include "hardware/hwcomposer.h"
#include "utils/RefBase.h"

namespace android {
class IGraphicBufferProducer;
}

namespace mozilla {

class MOZ_EXPORT GonkDisplayICS : public GonkDisplay {
public:
    GonkDisplayICS();
    ~GonkDisplayICS();

    virtual void SetEnabled(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback);

    virtual void* GetHWCDevice();

    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur);

    virtual ANativeWindowBuffer* DequeueBuffer();

    virtual bool QueueBuffer(ANativeWindowBuffer* handle);

    virtual void UpdateDispSurface(EGLDisplay dpy, EGLSurface sur);

    virtual void SetDispReleaseFd(int fd);

    virtual int GetPrevDispAcquireFd()
    {
        return -1;
    }

    virtual NativeData GetNativeData(
        GonkDisplay::DisplayType aDisplayType,
        android::IGraphicBufferProducer* aProducer = nullptr);

    virtual void NotifyBootAnimationStopped() {}

private:
    hw_module_t const*        mModule;
    hwc_composer_device_t*    mHwc;
    android::sp<android::FramebufferNativeWindow> mFBSurface;
};

}

#endif 
