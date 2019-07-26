















#ifndef mozilla_HwcComposer2D
#define mozilla_HwcComposer2D

#include "Composer2D.h"
#include "Layers.h"
#include <vector>
#include <list>

#include <hardware/hwcomposer.h>

#define MAX_HWC_LAYERS 15

namespace mozilla {

namespace layers {
class ContainerLayer;
class Layer;
}



typedef std::vector<hwc_rect_t> RectVector;
#if ANDROID_VERSION >= 18
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

    int Init(hwc_display_t aDisplay, hwc_surface_t aSurface);

    bool Initialized() const { return mHwc; }

    static HwcComposer2D* GetInstance();

    
    
    
    bool TryRender(layers::Layer* aRoot, const gfxMatrix& aGLWorldTransform) MOZ_OVERRIDE;

    bool Render(EGLDisplay dpy, EGLSurface sur);

private:
    void Prepare(buffer_handle_t fbHandle, int fence);
    bool Commit();
    bool TryHwComposition();
    bool ReallocLayerList();
    bool PrepareLayerList(layers::Layer* aContainer, const nsIntRect& aClip,
          const gfxMatrix& aParentTransform, const gfxMatrix& aGLWorldTransform);

    HwcDevice*              mHwc;
    HwcList*                mList;
    hwc_display_t           mDpy;
    hwc_surface_t           mSur;
    nsIntRect               mScreenRect;
    int                     mMaxLayerCount;
    bool                    mColorFill;
    bool                    mRBSwapSupport;
    
    
    std::list<RectVector>   mVisibleRegions;
    int                     mPrevRelFd[MAX_HWC_LAYERS + 1];
};

} 

#endif 
