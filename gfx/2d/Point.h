




#ifndef MOZILLA_GFX_POINT_H_
#define MOZILLA_GFX_POINT_H_

#include "Types.h"
#include "BasePoint.h"
#include "BaseSize.h"

namespace mozilla {
namespace gfx {

struct IntPoint :
  public BasePoint<int32_t, IntPoint> {
  typedef BasePoint<int32_t, IntPoint> Super;

  IntPoint() : Super() {}
  IntPoint(int32_t aX, int32_t aY) : Super(aX, aY) {}
};

struct Point :
  public BasePoint<Float, Point> {
  typedef BasePoint<Float, Point> Super;

  Point() : Super() {}
  Point(Float aX, Float aY) : Super(aX, aY) {}
  Point(const IntPoint& point) : Super(Float(point.x), Float(point.y)) {}
};

struct IntSize :
  public BaseSize<int32_t, IntSize> {
  typedef BaseSize<int32_t, IntSize> Super;

  IntSize() : Super() {}
  IntSize(int32_t aWidth, int32_t aHeight) : Super(aWidth, aHeight) {}
};

struct Size :
  public BaseSize<Float, Size> {
  typedef BaseSize<Float, Size> Super;

  Size() : Super() {}
  Size(Float aWidth, Float aHeight) : Super(aWidth, aHeight) {}
  explicit Size(const IntSize& size) :
    Super(Float(size.width), Float(size.height)) {}
};

}
}

#endif 
