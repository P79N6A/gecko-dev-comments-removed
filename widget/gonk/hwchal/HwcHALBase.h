

















#ifndef mozilla_HwcHALBase
#define mozilla_HwcHALBase

#include "mozilla/UniquePtr.h"
#include "nsRect.h"

#include <hardware/hwcomposer.h>

#ifndef HWC_BLIT
#if ANDROID_VERSION >= 21
#define HWC_BLIT 0xFF
#elif ANDROID_VERSION >= 17
#define HWC_BLIT (HWC_FRAMEBUFFER_TARGET + 1)
#else


#define HWC_BLIT 0xFF
#endif 
#endif 

namespace mozilla {

#if ANDROID_VERSION >= 17
using HwcDevice = hwc_composer_device_1_t;
using HwcList   = hwc_display_contents_1_t;
using HwcLayer  = hwc_layer_1_t;
#else
using HwcDevice = hwc_composer_device_t;
using HwcList   = hwc_layer_list_t;
using HwcLayer  = hwc_layer_t;
#endif




class HwcHALBase {

public:
    
    enum class QueryType {
        COLOR_FILL = 0x8,
        RB_SWAP = 0x40
    };

public:
    explicit HwcHALBase() = default;

    virtual ~HwcHALBase() {}

    
    
    
    static UniquePtr<HwcHALBase> CreateHwcHAL();

    
    virtual bool HasHwc() const = 0;

    
    virtual const hwc_rect_t GetHwcRect() const = 0;

    
    virtual void SetEGLInfo(hwc_display_t aDpy,
                            hwc_surface_t aSur) = 0;

    
    virtual bool Query(QueryType aType) = 0;

    
    virtual int Set(HwcList *aList,
                    uint32_t aDisp) = 0;

    
    virtual int ResetHwc() = 0;

    
    virtual int Prepare(HwcList *aList,
                        uint32_t aDisp,
                        buffer_handle_t aHandle,
                        int aFenceFd) = 0;

    
    virtual bool SupportTransparency() const = 0;

    
    virtual uint32_t GetGeometryChangedFlag(bool aGeometryChanged) const = 0;

    
    virtual void SetCrop(HwcLayer &aLayer,
                         const hwc_rect_t &aSrcCrop) const = 0;

protected:
    MOZ_CONSTEXPR static uint32_t HwcAPIVersion(uint32_t aMaj, uint32_t aMin) {
        
        return (((aMaj & 0xff) << 24) | ((aMin & 0xff) << 16) | (1 & 0xffff));
    }
};

} 

#endif
