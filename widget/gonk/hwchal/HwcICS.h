

















#ifndef mozilla_HwcICS
#define mozilla_HwcICS

#include "HwcHALBase.h"

namespace mozilla {

class HwcICS final : public HwcHALBase {
public:
    explicit HwcICS();

    virtual ~HwcICS();

    virtual bool HasHwc() const override { return static_cast<bool>(mHwc); }

    virtual const hwc_rect_t GetHwcRect() const override { return {0}; }

    virtual void SetEGLInfo(hwc_display_t aDpy,
                            hwc_surface_t aSur) override;

    virtual bool Query(QueryType aType) override;

    virtual int Set(HwcList *aList,
                    uint32_t aDisp) override;

    virtual int ResetHwc() override;

    virtual int Prepare(HwcList *aList,
                        uint32_t aDisp,
                        buffer_handle_t aHandle,
                        int aFenceFd) override;

    virtual bool SupportTransparency() const override;

    virtual uint32_t GetGeometryChangedFlag(bool aGeometryChanged) const override;

    virtual void SetCrop(HwcLayer &aLayer,
                         const hwc_rect_t &aSrcCrop) const override;

    virtual bool EnableVsync(bool aEnable) override;

    virtual bool RegisterHwcEventCallback(const HwcHALProcs_t &aProcs) override;

private:
    HwcDevice      *mHwc = nullptr;
    hwc_display_t   mDpy = nullptr;
    hwc_surface_t   mSur = nullptr;
};

} 

#endif 
