






#include "GrClipData.h"

#include "GrSurface.h"
#include "SkRect.h"









void GrClipData::getConservativeBounds(const GrSurface* surface,
                                       SkIRect* devResult,
                                       bool* isIntersectionOfRects) const {
    SkRect devBounds;

    fClipStack->getConservativeBounds(-fOrigin.fX,
                                      -fOrigin.fY,
                                      surface->width(),
                                      surface->height(),
                                      &devBounds,
                                      isIntersectionOfRects);

    devBounds.roundOut(devResult);
}
