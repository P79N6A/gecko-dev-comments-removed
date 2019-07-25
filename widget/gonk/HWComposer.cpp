















#include <string.h>
#include "HWComposer.h"
#include <hardware/hardware.h>
#include <EGL/egl.h>

namespace android {


HWComposer::HWComposer()
    : mModule(0), mHwc(0),
      mDpy(EGL_NO_DISPLAY), mSur(EGL_NO_SURFACE)
{
}

HWComposer::~HWComposer() {
    if (mHwc) {
        hwc_close(mHwc);
    }
}

int HWComposer::init() {
    int err = hw_get_module(HWC_HARDWARE_MODULE_ID, &mModule);
    LOGW_IF(err, "%s module not found", HWC_HARDWARE_MODULE_ID);
    if (err)
        return err;

    err = hwc_open(mModule, &mHwc);
    LOGE_IF(err, "%s device failed to initialize (%s)",
            HWC_HARDWARE_COMPOSER, strerror(-err));
    if (err) {
        mHwc = NULL;
        return err;
    }

    if (mHwc->registerProcs) {
        mCBContext.hwc = this;
        mHwc->registerProcs(mHwc, &mCBContext.procs);
    }

    return 0;
}

status_t HWComposer::swapBuffers(hwc_display_t dpy, hwc_surface_t surf) const {
    mHwc->prepare(mHwc, NULL);
    int err = mHwc->set(mHwc, dpy, surf, 0);
    return (status_t)err;
}


}; 
