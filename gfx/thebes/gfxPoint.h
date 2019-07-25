




































#ifndef GFX_POINT_H
#define GFX_POINT_H

#include "nsMathUtils.h"
#include "mozilla/BaseSize.h"
#include "mozilla/BasePoint.h"
#include "nsSize.h"
#include "nsPoint.h"

#include "gfxTypes.h"

typedef nsIntSize gfxIntSize;

struct THEBES_API gfxSize : public mozilla::BaseSize<gfxFloat, gfxSize> {
    typedef mozilla::BaseSize<gfxFloat, gfxSize> Super;

    gfxSize() : Super() {}
    gfxSize(gfxFloat aWidth, gfxFloat aHeight) : Super(aWidth, aHeight) {}
    gfxSize(const nsIntSize& aSize) : Super(aSize.width, aSize.height) {}
};

struct THEBES_API gfxPoint : public mozilla::BasePoint<gfxFloat, gfxPoint> {
    typedef mozilla::BasePoint<gfxFloat, gfxPoint> Super;

    gfxPoint() : Super() {}
    gfxPoint(gfxFloat aX, gfxFloat aY) : Super(aX, aY) {}
    gfxPoint(const nsIntPoint& aPoint) : Super(aPoint.x, aPoint.y) {}

    
    
    
    
    
    gfxPoint& Round() {
        x = NS_floor(x + 0.5);
        y = NS_floor(y + 0.5);
        return *this;
    }
};

#endif 
