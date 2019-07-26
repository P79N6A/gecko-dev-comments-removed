







#include "GrClipData.h"
#include "GrSurface.h"
#include "GrRect.h"









void GrClipData::getConservativeBounds(const GrSurface* surface,
                                       GrIRect* devResult,
                                       bool* isIntersectionOfRects) const {
    GrRect devBounds;

    fClipStack->getConservativeBounds(-fOrigin.fX,
                                      -fOrigin.fY,
                                      surface->width(),
                                      surface->height(),
                                      &devBounds,
                                      isIntersectionOfRects);

    devBounds.roundOut(devResult);
}

