




































#ifndef MOZILLA_GFX_RECT_H_
#define MOZILLA_GFX_RECT_H_

#include "BaseRect.h"
#include "BaseMargin.h"
#include "Point.h"

namespace mozilla {
namespace gfx {

struct Margin :
  public BaseMargin<Float, Margin> {
  typedef BaseMargin<Float, Margin> Super;

  
  Margin(const Margin& aMargin) : Super(aMargin) {}
  Margin(Float aLeft,  Float aTop, Float aRight, Float aBottom)
    : Super(aLeft, aTop, aRight, aBottom) {}
};

struct Rect :
    public BaseRect<Float, Rect, Point, Size, Margin> {
    typedef BaseRect<Float, Rect, Point, mozilla::gfx::Size, Margin> Super;

    Rect() : Super() {}
    Rect(Point aPos, mozilla::gfx::Size aSize) :
        Super(aPos, aSize) {}
    Rect(Float _x, Float _y, Float _width, Float _height) :
        Super(_x, _y, _width, _height) {}
};

struct IntRect :
    public BaseRect<int32_t, IntRect, IntPoint, IntSize, Margin> {
    typedef BaseRect<int32_t, IntRect, IntPoint, mozilla::gfx::IntSize, Margin> Super;

    IntRect() : Super() {}
    IntRect(IntPoint aPos, mozilla::gfx::IntSize aSize) :
        Super(aPos, aSize) {}
    IntRect(int32_t _x, int32_t _y, int32_t _width, int32_t _height) :
        Super(_x, _y, _width, _height) {}
};

}
}

#endif 
