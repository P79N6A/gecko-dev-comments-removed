




#ifndef GFX_POINTH3D_H
#define GFX_POINTH3D_H

#include "mozilla/gfx/BasePoint4D.h"
#include "gfxTypes.h"
#include "gfxPoint.h"

struct gfxPointH3D : public mozilla::gfx::BasePoint4D<float, gfxPointH3D> {
    typedef mozilla::gfx::BasePoint4D<float, gfxPointH3D> Super;

    gfxPointH3D() : Super() {}
    gfxPointH3D(float aX, float aY, float aZ, float aW) : Super(aX, aY, aZ, aW) {}

    bool HasPositiveWCoord() { return w > 0; }

    gfxPoint As2DPoint() {
      return gfxPoint(x / w, y / w);
    }
};

#endif 
