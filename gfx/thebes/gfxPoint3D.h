




































#ifndef GFX_POINT3D_H
#define GFX_POINT3D_H

#include "mozilla/gfx/BasePoint3D.h"
#include "gfxTypes.h"

struct THEBES_API gfxPoint3D : public mozilla::gfx::BasePoint3D<gfxFloat, gfxPoint3D> {
    typedef mozilla::gfx::BasePoint3D<gfxFloat, gfxPoint3D> Super;

    gfxPoint3D() : Super() {}
    gfxPoint3D(gfxFloat aX, gfxFloat aY, gfxFloat aZ) : Super(aX, aY, aZ) {}
};

#endif  
