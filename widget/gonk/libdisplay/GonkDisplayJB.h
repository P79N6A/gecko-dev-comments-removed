














#ifndef GONKDISPLAYJB_H
#define GONKDISPLAYJB_H

#include "GonkDisplay.h"
#include "FramebufferSurface.h"
#include "hardware/hwcomposer.h"
#include "hardware/power.h"
#include "ui/Fence.h"
#include "utils/RefBase.h"

namespace mozilla {

class MOZ_EXPORT GonkDisplayJB : public GonkDisplay {
public:
    GonkDisplayJB();
    ~GonkDisplayJB();

    virtual ANativeWindow* GetNativeWindow();

    virtual void SetEnabled(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback);

    virtual void* GetHWCDevice();

    virtual void* GetFBSurface();

    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur);

    virtual ANativeWindowBuffer* DequeueBuffer();

    virtual bool QueueBuffer(ANativeWindowBuffer* buf);

    virtual void UpdateFBSurface(EGLDisplay dpy, EGLSurface sur);

    virtual void SetFBReleaseFd(int fd);

    virtual int GetPrevFBAcquireFd();

    bool Post(buffer_handle_t buf, int fence);

private:
    hw_module_t const*        mModule;
    hw_module_t const*        mFBModule;
    hwc_composer_device_1_t*  mHwc;
    framebuffer_device_t*     mFBDevice;
    power_module_t*           mPowerModule;
    android::sp<android::FramebufferSurface> mFBSurface;
    android::sp<ANativeWindow> mSTClient;
    android::sp<android::IGraphicBufferAlloc> mAlloc;
    int mFence;
    hwc_display_contents_1_t* mList;
    uint32_t mWidth;
    uint32_t mHeight;
    OnEnabledCallbackType mEnabledCallback;
};

}

#endif 
