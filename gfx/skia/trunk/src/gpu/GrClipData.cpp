






#include "GrClipData.h"

#include "GrSurface.h"
#include "SkRect.h"









void GrClipData::getConservativeBounds(int width, int height,
                                       SkIRect* devResult,
                                       bool* isIntersectionOfRects) const {
    SkRect devBounds;

    fClipStack->getConservativeBounds(-fOrigin.fX,
                                      -fOrigin.fY,
                                      width,
                                      height,
                                      &devBounds,
                                      isIntersectionOfRects);

    devBounds.roundOut(devResult);
}
