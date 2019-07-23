




































#ifndef GFX_POINT_H
#define GFX_POINT_H

#include <math.h>

#include "gfxTypes.h"





struct THEBES_API gfxIntSize {
    PRInt32 width, height;

    gfxIntSize() {}
    gfxIntSize(PRInt32 _width, PRInt32 _height) : width(_width), height(_height) {}

    void SizeTo(PRInt32 _width, PRInt32 _height) {width = _width; height = _height;}

    int operator==(const gfxIntSize& s) const {
        return ((width == s.width) && (height == s.height));
    }
    int operator!=(const gfxIntSize& s) const {
        return ((width != s.width) || (height != s.height));
    }
    gfxIntSize operator+(const gfxIntSize& s) const {
        return gfxIntSize(width + s.width, height + s.height);
    }
    gfxIntSize operator-() const {
        return gfxIntSize(- width, - height);
    }
    gfxIntSize operator*(const PRInt32 v) const {
        return gfxIntSize(width * v, height * v);
    }
    gfxIntSize operator/(const PRInt32 v) const {
        return gfxIntSize(width / v, height / v);
    }
};

struct THEBES_API gfxSize {
    gfxFloat width, height;

    gfxSize() {}
    gfxSize(gfxFloat _width, gfxFloat _height) : width(_width), height(_height) {}
    gfxSize(const gfxIntSize& size) : width(size.width), height(size.height) {}

    void SizeTo(gfxFloat _width, gfxFloat _height) {width = _width; height = _height;}

    int operator==(const gfxSize& s) const {
        return ((width == s.width) && (height == s.height));
    }
    int operator!=(const gfxSize& s) const {
        return ((width != s.width) || (height != s.height));
    }
    gfxSize operator+(const gfxSize& s) const {
        return gfxSize(width + s.width, height + s.height);
    }
    gfxSize operator-() const {
        return gfxSize(- width, - height);
    }
    gfxSize operator*(const gfxFloat v) const {
        return gfxSize(width * v, height * v);
    }
    gfxSize operator/(const gfxFloat v) const {
        return gfxSize(width / v, height / v);
    }
};



struct THEBES_API gfxPoint {
    gfxFloat x, y;

    gfxPoint() { }
    gfxPoint(const gfxPoint& p) : x(p.x), y(p.y) {}
    gfxPoint(gfxFloat _x, gfxFloat _y) : x(_x), y(_y) {}

    void MoveTo(gfxFloat aX, gfxFloat aY) { x = aX; y = aY; }

    int operator==(const gfxPoint& p) const {
        return ((x == p.x) && (y == p.y));
    }
    int operator!=(const gfxPoint& p) const {
        return ((x != p.x) || (y != p.y));
    }
    gfxPoint operator+(const gfxPoint& p) const {
        return gfxPoint(x + p.x, y + p.y);
    }
    gfxPoint operator+(const gfxSize& s) const {
        return gfxPoint(x + s.width, y + s.height);
    }
    gfxPoint operator-(const gfxPoint& p) const {
        return gfxPoint(x - p.x, y - p.y);
    }
    gfxPoint operator-(const gfxSize& s) const {
        return gfxPoint(x - s.width, y - s.height);
    }
    gfxPoint operator-() const {
        return gfxPoint(- x, - y);
    }
    gfxPoint operator*(const gfxFloat v) const {
        return gfxPoint(x * v, y * v);
    }
    gfxPoint operator/(const gfxFloat v) const {
        return gfxPoint(x / v, y / v);
    }
    gfxPoint& Round() {
        x = ::floor(x + 0.5);
        y = ::floor(y + 0.5);
        return *this;
    }
};

#endif 
