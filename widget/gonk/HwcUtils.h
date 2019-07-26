















#ifndef mozilla_HwcUtils
#define mozilla_HwcUtils

#include "Layers.h"
#include <vector>
#include "hardware/hwcomposer.h"

namespace mozilla {

class HwcUtils {
public:

enum {
    HWC_USE_GPU = HWC_FRAMEBUFFER,
    HWC_USE_OVERLAY = HWC_OVERLAY,
    HWC_USE_COPYBIT
};


enum {
    
    
    
    HWC_COLOR_FILL = 0x8,
    
    
    HWC_FORMAT_RB_SWAP = 0x40
};

typedef std::vector<hwc_rect_t> RectVector;


















static bool CalculateClipRect(const gfxMatrix& aTransform,
                              const nsIntRect* aLayerClip,
                              nsIntRect aParentClip, nsIntRect* aRenderClip);


















static bool PrepareVisibleRegion(const nsIntRegion& aVisible,
                                 const gfxMatrix& aTransform,
                                 nsIntRect aClip, nsIntRect aBufferRect,
                                 RectVector* aVisibleRegionScreen);




















static bool PrepareLayerRects(nsIntRect aVisible, const gfxMatrix& aTransform,
                              nsIntRect aClip, nsIntRect aBufferRect,
                              hwc_rect_t* aSourceCrop,
                              hwc_rect_t* aVisibleRegionScreen);

};

} 

#endif
