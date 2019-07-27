

















#ifndef mozilla_HwcHAL
#define mozilla_HwcHAL

#include "HwcHALBase.h"

namespace mozilla {

class HwcHAL final : public HwcHALBase {
public:
    explicit HwcHAL();

    virtual ~HwcHAL();

    virtual bool HasHwc() const override { return static_cast<bool>(mHwc); }

    virtual const hwc_rect_t GetHwcRect() const override { return mHwcRect; }

    virtual void SetEGLInfo(hwc_display_t aDpy,
                            hwc_surface_t aSur) override { }

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
    void GetHwcAttributes();

    uint32_t GetAPIVersion() const;

private:
    HwcDevice  *mHwc = nullptr;
    hwc_rect_t  mHwcRect = {0};
};

} 

#endif 
