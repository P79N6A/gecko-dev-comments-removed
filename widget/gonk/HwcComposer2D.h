

















#ifndef mozilla_HwcComposer2D
#define mozilla_HwcComposer2D

#include "Composer2D.h"
#include "hwchal/HwcHALBase.h"              
#include "HwcUtils.h"                       
#include "Layers.h"
#include "mozilla/Mutex.h"
#include "mozilla/layers/FenceUtils.h"      
#include "mozilla/UniquePtr.h"              

#include <vector>
#include <list>

#if ANDROID_VERSION >= 17
#include <utils/Timers.h>
#endif

class nsScreenGonk;

namespace mozilla {

namespace gl {
    class GLContext;
}

namespace layers {
class CompositorParent;
class Layer;
}


















class HwcComposer2D : public mozilla::layers::Composer2D {
public:
    HwcComposer2D();
    virtual ~HwcComposer2D();

    static HwcComposer2D* GetInstance();

    
    
    
    virtual bool TryRenderWithHwc(layers::Layer* aRoot,
                                  nsIWidget* aWidget,
                                  bool aGeometryChanged) override;

    virtual bool Render(nsIWidget* aWidget) override;

    virtual bool HasHwc() override { return mHal->HasHwc(); }

    bool EnableVsync(bool aEnable);
#if ANDROID_VERSION >= 17
    bool RegisterHwcEventCallback();
    void Vsync(int aDisplay, int64_t aTimestamp);
    void Invalidate();
    void Hotplug(int aDisplay, int aConnected);
#endif
    void SetCompositorParent(layers::CompositorParent* aCompositorParent);

private:
    void Reset();
    void Prepare(buffer_handle_t dispHandle, int fence, nsScreenGonk* screen);
    bool Commit(nsScreenGonk* aScreen);
    bool TryHwComposition(nsScreenGonk* aScreen);
    bool ReallocLayerList();
    bool PrepareLayerList(layers::Layer* aContainer, const nsIntRect& aClip,
          const gfx::Matrix& aParentTransform);
    void SendtoLayerScope();

    UniquePtr<HwcHALBase>   mHal;
    HwcList*                mList;
    nsIntRect               mScreenRect;
    int                     mMaxLayerCount;
    bool                    mColorFill;
    bool                    mRBSwapSupport;
    
    
    std::list<HwcUtils::RectVector>   mVisibleRegions;
    layers::FenceHandle mPrevRetireFence;
    layers::FenceHandle mPrevDisplayFence;
    nsTArray<layers::LayerComposite*> mHwcLayerMap;
    bool                    mPrepared;
    bool                    mHasHWVsync;
    nsRefPtr<layers::CompositorParent> mCompositorParent;
    Mutex mLock;
};

} 

#endif 
