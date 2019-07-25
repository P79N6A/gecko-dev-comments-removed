















#ifndef ANDROID_SF_HWCOMPOSER_H
#define ANDROID_SF_HWCOMPOSER_H

#include <hardware/hwcomposer.h>
#include <utils/Vector.h>

namespace android {


class String8;

class HWComposer
{
public:

    HWComposer();
    ~HWComposer();

    int init();

    
    status_t swapBuffers(hwc_display_t dpy, hwc_surface_t surf) const;

private:
    struct cb_context {
        hwc_procs_t procs;
        HWComposer* hwc;
    };
    void invalidate();

    hw_module_t const*      mModule;
    hwc_composer_device_t*  mHwc;
    hwc_display_t           mDpy;
    hwc_surface_t           mSur;
    cb_context              mCBContext;
};



}; 

#endif 
