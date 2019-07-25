




































#ifndef GFX_RECT_H
#define GFX_RECT_H

#include "nsAlgorithm.h"
#include "gfxTypes.h"
#include "gfxPoint.h"
#include "gfxCore.h"
#include "nsDebug.h"
#include "mozilla/BaseMargin.h"
#include "mozilla/BaseRect.h"
#include "nsRect.h"

struct gfxMargin : public mozilla::BaseMargin<gfxFloat, gfxMargin> {
  typedef mozilla::BaseMargin<gfxFloat, gfxMargin> Super;

  
  gfxMargin() : Super() {}
  gfxMargin(const gfxMargin& aMargin) : Super(aMargin) {}
  gfxMargin(gfxFloat aLeft,  gfxFloat aTop, gfxFloat aRight, gfxFloat aBottom)
    : Super(aLeft, aTop, aRight, aBottom) {}
};

namespace mozilla {
    namespace css {
        enum Corner {
            
            eCornerTopLeft = 0,
            eCornerTopRight = 1,
            eCornerBottomRight = 2,
            eCornerBottomLeft = 3,
            eNumCorners = 4
        };
    }
}
#define NS_CORNER_TOP_LEFT mozilla::css::eCornerTopLeft
#define NS_CORNER_TOP_RIGHT mozilla::css::eCornerTopRight
#define NS_CORNER_BOTTOM_RIGHT mozilla::css::eCornerBottomRight
#define NS_CORNER_BOTTOM_LEFT mozilla::css::eCornerBottomLeft
#define NS_NUM_CORNERS mozilla::css::eNumCorners

#define NS_FOR_CSS_CORNERS(var_)                         \
    for (mozilla::css::Corner var_ = NS_CORNER_TOP_LEFT; \
         var_ <= NS_CORNER_BOTTOM_LEFT;                  \
         var_++)

static inline mozilla::css::Corner operator++(mozilla::css::Corner& corner, int) {
    NS_PRECONDITION(corner >= NS_CORNER_TOP_LEFT &&
                    corner < NS_NUM_CORNERS, "Out of range corner");
    corner = mozilla::css::Corner(corner + 1);
    return corner;
}

struct THEBES_API gfxRect :
    public mozilla::BaseRect<gfxFloat, gfxRect, gfxPoint, gfxSize, gfxMargin> {
    typedef mozilla::BaseRect<gfxFloat, gfxRect, gfxPoint, gfxSize, gfxMargin> Super;

    gfxRect() : Super() {}
    gfxRect(const gfxPoint& aPos, const gfxSize& aSize) :
        Super(aPos, aSize) {}
    gfxRect(gfxFloat aX, gfxFloat aY, gfxFloat aWidth, gfxFloat aHeight) :
        Super(aX, aY, aWidth, aHeight) {}
    gfxRect(const nsIntRect& aRect) :
        Super(aRect.x, aRect.y, aRect.width, aRect.height) {}

    





    PRBool WithinEpsilonOfIntegerPixels(gfxFloat aEpsilon) const;

    
    
    
    
    
    
    
    
    
    
    
    void Round();

    
    
    void RoundIn();
    
    
    
    void RoundOut();

    gfxPoint AtCorner(mozilla::css::Corner corner) const {
        switch (corner) {
            case NS_CORNER_TOP_LEFT: return TopLeft();
            case NS_CORNER_TOP_RIGHT: return TopRight();
            case NS_CORNER_BOTTOM_RIGHT: return BottomRight();
            case NS_CORNER_BOTTOM_LEFT: return BottomLeft();
            default:
                NS_ERROR("Invalid corner!");
                break;
        }
        return gfxPoint(0.0, 0.0);
    }

    gfxPoint CCWCorner(mozilla::css::Side side) const {
        switch (side) {
            case NS_SIDE_TOP: return TopLeft();
            case NS_SIDE_RIGHT: return TopRight();
            case NS_SIDE_BOTTOM: return BottomRight();
            case NS_SIDE_LEFT: return BottomLeft();
            default:
                NS_ERROR("Invalid side!");
                break;
        }
        return gfxPoint(0.0, 0.0);
    }

    gfxPoint CWCorner(mozilla::css::Side side) const {
        switch (side) {
            case NS_SIDE_TOP: return TopRight();
            case NS_SIDE_RIGHT: return BottomRight();
            case NS_SIDE_BOTTOM: return BottomLeft();
            case NS_SIDE_LEFT: return TopLeft();
            default:
                NS_ERROR("Invalid side!");
                break;
        }
        return gfxPoint(0.0, 0.0);
    }

    



    void Condition();

    void Scale(gfxFloat k) {
        NS_ASSERTION(k >= 0.0, "Invalid (negative) scale factor");
        x *= k;
        y *= k;
        width *= k;
        height *= k;
    }

    void Scale(gfxFloat sx, gfxFloat sy) {
        NS_ASSERTION(sx >= 0.0, "Invalid (negative) scale factor");
        NS_ASSERTION(sy >= 0.0, "Invalid (negative) scale factor");
        x *= sx;
        y *= sy;
        width *= sx;
        height *= sy;
    }

    void ScaleInverse(gfxFloat k) {
        NS_ASSERTION(k > 0.0, "Invalid (negative) scale factor");
        x /= k;
        y /= k;
        width /= k;
        height /= k;
    }
};

struct THEBES_API gfxCornerSizes {
    gfxSize sizes[NS_NUM_CORNERS];

    gfxCornerSizes () { }

    gfxCornerSizes (gfxFloat v) {
        for (int i = 0; i < NS_NUM_CORNERS; i++)
            sizes[i].SizeTo(v, v);
    }

    gfxCornerSizes (gfxFloat tl, gfxFloat tr, gfxFloat br, gfxFloat bl) {
        sizes[NS_CORNER_TOP_LEFT].SizeTo(tl, tl);
        sizes[NS_CORNER_TOP_RIGHT].SizeTo(tr, tr);
        sizes[NS_CORNER_BOTTOM_RIGHT].SizeTo(br, br);
        sizes[NS_CORNER_BOTTOM_LEFT].SizeTo(bl, bl);
    }

    gfxCornerSizes (const gfxSize& tl, const gfxSize& tr, const gfxSize& br, const gfxSize& bl) {
        sizes[NS_CORNER_TOP_LEFT] = tl;
        sizes[NS_CORNER_TOP_RIGHT] = tr;
        sizes[NS_CORNER_BOTTOM_RIGHT] = br;
        sizes[NS_CORNER_BOTTOM_LEFT] = bl;
    }

    const gfxSize& operator[] (mozilla::css::Corner index) const {
        return sizes[index];
    }

    gfxSize& operator[] (mozilla::css::Corner index) {
        return sizes[index];
    }

    const gfxSize TopLeft() const { return sizes[NS_CORNER_TOP_LEFT]; }
    gfxSize& TopLeft() { return sizes[NS_CORNER_TOP_LEFT]; }

    const gfxSize TopRight() const { return sizes[NS_CORNER_TOP_RIGHT]; }
    gfxSize& TopRight() { return sizes[NS_CORNER_TOP_RIGHT]; }

    const gfxSize BottomLeft() const { return sizes[NS_CORNER_BOTTOM_LEFT]; }
    gfxSize& BottomLeft() { return sizes[NS_CORNER_BOTTOM_LEFT]; }

    const gfxSize BottomRight() const { return sizes[NS_CORNER_BOTTOM_RIGHT]; }
    gfxSize& BottomRight() { return sizes[NS_CORNER_BOTTOM_RIGHT]; }
};
#endif 
