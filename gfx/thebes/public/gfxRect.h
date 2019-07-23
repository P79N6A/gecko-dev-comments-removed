




































#ifndef GFX_RECT_H
#define GFX_RECT_H

#include "gfxTypes.h"
#include "gfxPoint.h"

struct THEBES_API gfxRect {
    
    gfxPoint pos;
    gfxSize size;

    gfxRect() {}
    gfxRect(const gfxRect& s) : pos(s.pos), size(s.size) {}
    gfxRect(const gfxPoint& _pos, const gfxSize& _size) : pos(_pos), size(_size) {}
    gfxRect(gfxFloat _x, gfxFloat _y, gfxFloat _width, gfxFloat _height) :
        pos(_x, _y), size(_width, _height) {}

    int operator==(const gfxRect& s) const {
        return (pos == s.pos) && (size == s.size);
    }
    int operator!=(const gfxRect& s) const {
        return (pos != s.pos) || (size != s.size);
    }

    const gfxRect& MoveBy(const gfxPoint& aPt) {
        pos = pos + aPt;
        return *this;
    }
    gfxRect operator+(const gfxPoint& aPt) const {
        return gfxRect(pos + aPt, size);
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

    
    
    
    
    void Round();

    
    gfxPoint TopLeft() const { return gfxPoint(pos); }
    gfxPoint TopRight() const { return pos + gfxSize(size.width, 0.0); }
    gfxPoint BottomLeft() const { return pos + gfxSize(0.0, size.height); }
    gfxPoint BottomRight() const { return pos + size; }

    



    void Condition();

    void Scale(gfxFloat k) {
        NS_ASSERTION(k >= 0.0, "Invalid (negative) scale factor");
        pos.x *= k;
        pos.y *= k;
        size.width *= k;
        size.height *= k;
    }

    void ScaleInverse(gfxFloat k) {
        NS_ASSERTION(k > 0.0, "Invalid (negative) scale factor");
        pos.x /= k;
        pos.y /= k;
        size.width /= k;
        size.height /= k;
    }
};

#endif 
