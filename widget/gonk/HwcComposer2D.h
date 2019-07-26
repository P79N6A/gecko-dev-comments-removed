















#ifndef mozilla_HwcComposer2D
#define mozilla_HwcComposer2D

#include "Composer2D.h"
#include "HWComposer.h"
#include "Layers.h"
#include <vector>
#include <list>

namespace mozilla {

namespace layers {
class ContainerLayer;
class Layer;
}



typedef std::vector<hwc_rect_t> RectVector;

class HwcComposer2D : public android::HWComposer,
                      public mozilla::layers::Composer2D {
public:
    HwcComposer2D();
    virtual ~HwcComposer2D();

    int Init(hwc_display_t aDisplay, hwc_surface_t aSurface);

    bool Initialized() const { return mHwc; }

    static HwcComposer2D* GetInstance();

    
    
    
    bool TryRender(layers::Layer* aRoot, const gfxMatrix& aGLWorldTransform) MOZ_OVERRIDE;

private:
    bool ReallocLayerList();
    bool PrepareLayerList(layers::Layer* aContainer, const nsIntRect& aClip,
          const gfxMatrix& aParentTransform, const gfxMatrix& aGLWorldTransform);

    hwc_layer_list_t*       mList;
    nsIntRect               mScreenRect;
    int                     mMaxLayerCount;
    bool                    mColorFill;
    
    
    std::list<RectVector>   mVisibleRegions;
};

} 

#endif 
