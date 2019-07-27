

















#include "HwcICS.h"
#include "cutils/properties.h"
#include "libdisplay/GonkDisplay.h"

namespace mozilla {

HwcICS::HwcICS()
    : HwcHALBase()
{
    mHwc = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!mHwc) {
        printf_stderr("HwcHAL Error: Cannot load hwcomposer");
    }
}

HwcICS::~HwcICS()
{
    mHwc = nullptr;
    mDpy = nullptr;
    mSur = nullptr;
}

void
HwcICS::SetEGLInfo(hwc_display_t aDpy,
                   hwc_surface_t aSur)
{
    mDpy = aDpy;
    mSur = aSur;
}

bool
HwcICS::Query(QueryType aType)
{
    bool value = false;
    switch (aType) {
        case QueryType::COLOR_FILL: {
            char propValue[PROPERTY_VALUE_MAX];
            property_get("ro.display.colorfill", propValue, "0");
            value = (atoi(propValue) == 1) ? true : false;
            break;
        }
        case QueryType::RB_SWAP:
            value = true;
            break;

        default:
            value = false;
    }
    return value;
}

int
HwcICS::Set(HwcList* aList,
            uint32_t aDisp)
{
    if (!mHwc) {
        return -1;
    }
    return mHwc->set(mHwc, mDpy, mSur, aList);
}

int
HwcICS::ResetHwc()
{
    return -1;
}

int
HwcICS::Prepare(HwcList *aList,
                uint32_t aDisp,
                buffer_handle_t aHandle,
                int aFenceFd)
{
    return mHwc->prepare(mHwc, aList);
}

bool
HwcICS::SupportTransparency() const
{
    return false;
}

uint32_t
HwcICS::GetGeometryChangedFlag(bool aGeometryChanged) const
{
    return HWC_GEOMETRY_CHANGED;
}

void
HwcICS::SetCrop(HwcLayer& aLayer,
                const hwc_rect_t &aSrcCrop) const
{
    aLayer.sourceCrop = aSrcCrop;
}

bool
HwcICS::EnableVsync(bool aEnable)
{
    return false;
}

bool
HwcICS::RegisterHwcEventCallback(const HwcHALProcs_t &aProcs)
{
    return false;
}


UniquePtr<HwcHALBase>
HwcHALBase::CreateHwcHAL()
{
    return Move(MakeUnique<HwcICS>());
}

} 
