

















#include "HwcHAL.h"
#include "libdisplay/GonkDisplay.h"
#include "mozilla/Assertions.h"

namespace mozilla {

HwcHAL::HwcHAL()
    : HwcHALBase()
{
    
    
    mHwc = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!mHwc) {
        printf_stderr("HwcHAL Error: Cannot load hwcomposer");
        return;
    }

    GetHwcAttributes();
}

HwcHAL::~HwcHAL()
{
    mHwc = nullptr;
}

bool
HwcHAL::Query(QueryType aType)
{
    if (!mHwc || !mHwc->query) {
        return false;
    }

    bool value = false;
    int supported = 0;
    if (mHwc->query(mHwc, static_cast<int>(aType), &supported) == 0) {
        value = !!supported;
    }
    return value;
}

int
HwcHAL::Set(HwcList *aList,
            uint32_t aDisp)
{
    MOZ_ASSERT(mHwc);
    if (!mHwc) {
        return -1;
    }

    HwcList *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[aDisp] = aList;
    return mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
}

int
HwcHAL::ResetHwc()
{
    return Set(nullptr, HWC_DISPLAY_PRIMARY);
}

int
HwcHAL::Prepare(HwcList *aList,
                uint32_t aDisp,
                buffer_handle_t aHandle,
                int aFenceFd)
{
    MOZ_ASSERT(mHwc);
    if (!mHwc) {
        printf_stderr("HwcHAL Error: HwcDevice doesn't exist. A fence might be leaked.");
        return -1;
    }

    HwcList *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[aDisp] = aList;
#if ANDROID_VERSION >= 18
    aList->outbufAcquireFenceFd = -1;
    aList->outbuf = nullptr;
#endif
    aList->retireFenceFd = -1;

    const auto idx = aList->numHwLayers - 1;
    aList->hwLayers[idx].hints = 0;
    aList->hwLayers[idx].flags = 0;
    aList->hwLayers[idx].transform = 0;
    aList->hwLayers[idx].handle = aHandle;
    aList->hwLayers[idx].blending = HWC_BLENDING_PREMULT;
    aList->hwLayers[idx].compositionType = HWC_FRAMEBUFFER_TARGET;
    SetCrop(aList->hwLayers[idx], mHwcRect);
    aList->hwLayers[idx].displayFrame = mHwcRect;
    aList->hwLayers[idx].visibleRegionScreen.numRects = 1;
    aList->hwLayers[idx].visibleRegionScreen.rects = &aList->hwLayers[idx].displayFrame;
    aList->hwLayers[idx].acquireFenceFd = aFenceFd;
    aList->hwLayers[idx].releaseFenceFd = -1;
#if ANDROID_VERSION >= 18
    aList->hwLayers[idx].planeAlpha = 0xFF;
#endif
    return mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
}

bool
HwcHAL::SupportTransparency() const
{
#if ANDROID_VERSION >= 18
    return true;
#endif
    return false;
}

uint32_t
HwcHAL::GetGeometryChangedFlag(bool aGeometryChanged) const
{
#if ANDROID_VERSION >= 19
    return aGeometryChanged ? HWC_GEOMETRY_CHANGED : 0;
#else
    return HWC_GEOMETRY_CHANGED;
#endif
}

void
HwcHAL::SetCrop(HwcLayer &aLayer,
                const hwc_rect_t &aSrcCrop) const
{
    if (GetAPIVersion() >= HwcAPIVersion(1, 3)) {
#if ANDROID_VERSION >= 19
        aLayer.sourceCropf.left = aSrcCrop.left;
        aLayer.sourceCropf.top = aSrcCrop.top;
        aLayer.sourceCropf.right = aSrcCrop.right;
        aLayer.sourceCropf.bottom = aSrcCrop.bottom;
#endif
    } else {
        aLayer.sourceCrop = aSrcCrop;
    }
}

void
HwcHAL::GetHwcAttributes()
{
    int32_t values[2];
    const uint32_t attrs[] = {
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_NO_ATTRIBUTE
    };

    mHwc->getDisplayAttributes(mHwc, 0, 0, attrs, values);
    mHwcRect = {0, 0, values[0], values[1]};
}

uint32_t
HwcHAL::GetAPIVersion() const
{
    if (!mHwc) {
        
        return 1;
    }
    return mHwc->common.version;
}


UniquePtr<HwcHALBase>
HwcHALBase::CreateHwcHAL()
{
    return Move(MakeUnique<HwcHAL>());
}

} 
