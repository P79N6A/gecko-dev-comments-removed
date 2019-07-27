















#ifndef mozilla_HwcComposer2D
#define mozilla_HwcComposer2D

#include "Composer2D.h"
#include "Layers.h"
#include "mozilla/Mutex.h"

#include <vector>
#include <list>

#include <hardware/hwcomposer.h>
#if ANDROID_VERSION >= 17
#include <ui/Fence.h>
#include <utils/Timers.h>
#endif

namespace mozilla {

namespace gl {
    class GLContext;
}

namespace layers {
class CompositorParent;
class ContainerLayer;
class Layer;
}



typedef std::vector<hwc_rect_t> RectVector;
#if ANDROID_VERSION >= 17
typedef hwc_composer_device_1_t HwcDevice;
typedef hwc_display_contents_1_t HwcList;
typedef hwc_layer_1_t HwcLayer;
#else
typedef hwc_composer_device_t HwcDevice;
typedef hwc_layer_list_t HwcList;
typedef hwc_layer_t HwcLayer;
#endif


















class HwcComposer2D : public mozilla::layers::Composer2D {
public:
    HwcComposer2D();
    virtual ~HwcComposer2D();

    static HwcComposer2D* GetInstance();

    
    
    
    virtual bool TryRenderWithHwc(layers::Layer* aRoot,
                                  bool aGeometryChanged) override;

    virtual bool Render() override;

    virtual bool HasHwc() override { return mHwc; }

    bool EnableVsync(bool aEnable);
#if ANDROID_VERSION >= 17
    bool RegisterHwcEventCallback();
    void Vsync(int aDisplay, int64_t aTimestamp);
    void Invalidate();
#endif
    void SetCompositorParent(layers::CompositorParent* aCompositorParent);

    
    
    void SetEGLInfo(hwc_display_t aDisplay, hwc_surface_t aSurface, gl::GLContext* aGLContext);

private:
    void Reset();
    void Prepare(buffer_handle_t fbHandle, int fence);
    bool Commit();
    bool TryHwComposition();
    bool ReallocLayerList();
    bool PrepareLayerList(layers::Layer* aContainer, const nsIntRect& aClip,
          const gfx::Matrix& aParentTransform);
    void setCrop(HwcLayer* layer, hwc_rect_t srcCrop);
    void setHwcGeometry(bool aGeometryChanged);
    void SendtoLayerScope();

    HwcDevice*              mHwc;
    HwcList*                mList;
    hwc_display_t           mDpy; 
    hwc_surface_t           mSur; 
    gl::GLContext*          mGLContext; 
    nsIntRect               mScreenRect;
    int                     mMaxLayerCount;
    bool                    mColorFill;
    bool                    mRBSwapSupport;
    
    
    std::list<RectVector>   mVisibleRegions;
#if ANDROID_VERSION >= 17
    android::sp<android::Fence> mPrevRetireFence;
    android::sp<android::Fence> mPrevDisplayFence;
#endif
    nsTArray<layers::LayerComposite*> mHwcLayerMap;
    bool                    mPrepared;
    bool                    mHasHWVsync;
    nsRefPtr<layers::CompositorParent> mCompositorParent;
    Mutex mLock;
};

} 

#endif 
