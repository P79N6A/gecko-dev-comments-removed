






#include "mozilla/WidgetUtils.h"

namespace mozilla {

gfxMatrix
ComputeTransformForRotation(const nsIntRect& aBounds,
                              ScreenRotation aRotation)
{
    gfxMatrix transform;
    switch (aRotation) {
    case ROTATION_0:
        break;
    case ROTATION_90:
        transform.Translate(gfxPoint(aBounds.width, 0));
        transform.Rotate(M_PI / 2);
        break;
    case ROTATION_180:
        transform.Translate(gfxPoint(aBounds.width, aBounds.height));
        transform.Rotate(M_PI);
        break;
    case ROTATION_270:
        transform.Translate(gfxPoint(0, aBounds.height));
        transform.Rotate(M_PI * 3 / 2);
        break;
    default:
        MOZ_NOT_REACHED("Unknown rotation");
        break;
    }
    return transform;
}

} 
