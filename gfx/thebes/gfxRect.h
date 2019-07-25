




































#ifndef GFX_RECT_H
#define GFX_RECT_H

#include "gfxTypes.h"
#include "gfxPoint.h"
#include "gfxCore.h"
#include "nsDebug.h" 

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

struct THEBES_API gfxRect {
    
    gfxPoint pos;
    gfxSize size;

    gfxRect() {}
    gfxRect(const gfxPoint& _pos, const gfxSize& _size) : pos(_pos), size(_size) {}
    gfxRect(gfxFloat _x, gfxFloat _y, gfxFloat _width, gfxFloat _height) :
        pos(_x, _y), size(_width, _height) {}

    int operator==(const gfxRect& s) const {
        return (pos == s.pos) && (size == s.size);
    }
    int operator!=(const gfxRect& s) const {
        return (pos != s.pos) || (size != s.size);
    }

    void MoveTo(const gfxPoint& aPt) { pos = aPt; }
    const gfxRect& MoveBy(const gfxPoint& aPt) {
        pos = pos + aPt;
        return *this;
    }
    void SizeTo(const gfxSize& aSize) { size = aSize; }

    gfxRect operator+(const gfxPoint& aPt) const {
        return gfxRect(pos + aPt, size);
    }
    gfxRect operator-(const gfxPoint& aPt) const {
        return gfxRect(pos - aPt, size);
    }
    gfxRect operator+(const gfxSize& aSize) const {
        return gfxRect(pos + gfxPoint(aSize.width, aSize.height), size);
    }
    gfxRect operator-(const gfxSize& aSize) const {
        return gfxRect(pos - gfxPoint(aSize.width, aSize.height), size);
    }
    gfxRect operator*(const gfxFloat aScale) const {
        return gfxRect(pos * aScale, size * aScale);
    }

    const gfxRect& operator+=(const gfxPoint& aPt) {
        pos += aPt;
        return *this;
    }
    const gfxRect& operator-=(const gfxPoint& aPt) {
        pos -= aPt;
        return *this;
    }
    const gfxRect& operator+=(const gfxSize& aSize) {
        pos += gfxPoint(aSize.width, aSize.height);
        return *this;
    }
    const gfxRect& operator-=(const gfxSize& aSize) {
        pos -= gfxPoint(aSize.width, aSize.height);
        return *this;
    }

    gfxFloat Width() const { return size.width; }
    gfxFloat Height() const { return size.height; }
    gfxFloat X() const { return pos.x; }
    gfxFloat Y() const { return pos.y; }
    gfxFloat XMost() const { return pos.x + size.width; }
    gfxFloat YMost() const { return pos.y + size.height; }

    PRBool IsEmpty() const { return size.width <= 0 || size.height <= 0; }
    gfxRect Intersect(const gfxRect& aRect) const;
    gfxRect Union(const gfxRect& aRect) const;
    PRBool Contains(const gfxRect& aRect) const;
    PRBool Contains(const gfxPoint& aPoint) const;

    





    PRBool WithinEpsilonOfIntegerPixels(gfxFloat aEpsilon) const;

    gfxSize Size() const { return size; }

    void Inset(gfxFloat k) {
        pos.x += k;
        pos.y += k;
        size.width = PR_MAX(0.0, size.width - k * 2.0);
        size.height = PR_MAX(0.0, size.height - k * 2.0);
    }

    void Inset(gfxFloat top, gfxFloat right, gfxFloat bottom, gfxFloat left) {
        pos.x += left;
        pos.y += top;
        size.width = PR_MAX(0.0, size.width - (right+left));
        size.height = PR_MAX(0.0, size.height - (bottom+top));
    }

    void Inset(const gfxFloat *sides) {
        Inset(sides[0], sides[1], sides[2], sides[3]);
    }

    void Inset(const gfxIntSize& aSize) {
        Inset(aSize.height, aSize.width, aSize.height, aSize.width);
    }

    void Outset(gfxFloat k) {
        pos.x -= k;
        pos.y -= k;
        size.width = PR_MAX(0.0, size.width + k * 2.0);
        size.height = PR_MAX(0.0, size.height + k * 2.0);
    }

    void Outset(gfxFloat top, gfxFloat right, gfxFloat bottom, gfxFloat left) {
        pos.x -= left;
        pos.y -= top;
        size.width = PR_MAX(0.0, size.width + (right+left));
        size.height = PR_MAX(0.0, size.height + (bottom+top));
    }

    void Outset(const gfxFloat *sides) {
        Outset(sides[0], sides[1], sides[2], sides[3]);
    }

    void Outset(const gfxIntSize& aSize) {
        Outset(aSize.height, aSize.width, aSize.height, aSize.width);
    }

    
    
    
    
    
    
    
    
    
    
    
    void Round();

    
    
    void RoundIn();
    
    
    
    void RoundOut();

    
    gfxPoint TopLeft() const { return gfxPoint(pos); }
    gfxPoint TopRight() const { return pos + gfxPoint(size.width, 0.0); }
    gfxPoint BottomLeft() const { return pos + gfxPoint(0.0, size.height); }
    gfxPoint BottomRight() const { return pos + gfxPoint(size.width, size.height); }
    gfxPoint Center() const { return pos + gfxPoint(size.width, size.height)/2.0; }

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
        pos.x *= k;
        pos.y *= k;
        size.width *= k;
        size.height *= k;
    }

    void Scale(gfxFloat sx, gfxFloat sy) {
        NS_ASSERTION(sx >= 0.0, "Invalid (negative) scale factor");
        NS_ASSERTION(sy >= 0.0, "Invalid (negative) scale factor");
        pos.x *= sx;
        pos.y *= sy;
        size.width *= sx;
        size.height *= sy;
    }

    void ScaleInverse(gfxFloat k) {
        NS_ASSERTION(k > 0.0, "Invalid (negative) scale factor");
        pos.x /= k;
        pos.y /= k;
        size.width /= k;
        size.height /= k;
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
