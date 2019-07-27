















#ifndef mozilla_HwcUtils
#define mozilla_HwcUtils

#include "Layers.h"
#include <vector>
#include "hardware/hwcomposer.h"

namespace mozilla {

namespace gfx {
class Matrix;
}

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


















static bool CalculateClipRect(const gfx::Matrix& aTransform,
                              const nsIntRect* aLayerClip,
                              nsIntRect aParentClip, nsIntRect* aRenderClip);


















static bool PrepareVisibleRegion(const nsIntRegion& aVisible,
                                 const gfx::Matrix& aTransform,
                                 nsIntRect aClip, nsIntRect aBufferRect,
                                 RectVector* aVisibleRegionScreen);





















static bool PrepareLayerRects(nsIntRect aVisible, const gfx::Matrix& aTransform,
                              nsIntRect aClip, nsIntRect aBufferRect,
                              bool aYFlipped,
                              hwc_rect_t* aSourceCrop,
                              hwc_rect_t* aVisibleRegionScreen);


static nsIntRect HwcToIntRect(hwc_rect_t aRect);

};

} 

#endif
