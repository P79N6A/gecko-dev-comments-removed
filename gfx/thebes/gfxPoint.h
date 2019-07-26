




#ifndef GFX_POINT_H
#define GFX_POINT_H

#include "nsMathUtils.h"
#include "mozilla/gfx/BaseSize.h"
#include "mozilla/gfx/BasePoint.h"
#include "nsSize.h"
#include "nsPoint.h"

#include "gfxTypes.h"

typedef nsIntSize gfxIntSize;

struct THEBES_API gfxSize : public mozilla::gfx::BaseSize<gfxFloat, gfxSize> {
    typedef mozilla::gfx::BaseSize<gfxFloat, gfxSize> Super;

    gfxSize() : Super() {}
    gfxSize(gfxFloat aWidth, gfxFloat aHeight) : Super(aWidth, aHeight) {}
    gfxSize(const nsIntSize& aSize) : Super(aSize.width, aSize.height) {}
};

struct THEBES_API gfxPoint : public mozilla::gfx::BasePoint<gfxFloat, gfxPoint> {
    typedef mozilla::gfx::BasePoint<gfxFloat, gfxPoint> Super;

    gfxPoint() : Super() {}
    gfxPoint(gfxFloat aX, gfxFloat aY) : Super(aX, aY) {}
    gfxPoint(const nsIntPoint& aPoint) : Super(aPoint.x, aPoint.y) {}

    
    
    
    gfxPoint& Round() {
        x = floor(x + 0.5);
        y = floor(y + 0.5);
        return *this;
    }

    bool WithinEpsilonOf(const gfxPoint& aPoint, gfxFloat aEpsilon) {
        return fabs(aPoint.x - x) < aEpsilon && fabs(aPoint.y - y) < aEpsilon;
    }
};

inline gfxPoint
operator*(const gfxPoint& aPoint, const gfxSize& aSize)
{
  return gfxPoint(aPoint.x * aSize.width, aPoint.y * aSize.height);
}

inline gfxPoint
operator/(const gfxPoint& aPoint, const gfxSize& aSize)
{
  return gfxPoint(aPoint.x / aSize.width, aPoint.y / aSize.height);
}

inline gfxSize
operator/(gfxFloat aValue, const gfxSize& aSize)
{
  return gfxSize(aValue / aSize.width, aValue / aSize.height);
}

#endif 
