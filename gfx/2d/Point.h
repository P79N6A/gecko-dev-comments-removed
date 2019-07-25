




































#ifndef MOZILLA_GFX_POINT_H_
#define MOZILLA_GFX_POINT_H_

#include "Types.h"
#include "BasePoint.h"
#include "BaseSize.h"

namespace mozilla {
namespace gfx {

struct Point :
  public BasePoint<Float, Point> {
  typedef BasePoint<Float, Point> Super;
  Point() : Super() {}
  Point(Float aX, Float aY) : Super(aX, aY) {}
};

struct IntPoint :
  public BasePoint<int32_t, Point> {
  typedef BasePoint<int32_t, Point> Super;
  IntPoint() : Super() {}
  IntPoint(int32_t aX, int32_t aY) : Super(aX, aY) {}
};

struct Size :
  public BaseSize<Float, Size> {
  typedef BaseSize<Float, Size> Super;

  Size() : Super() {}
  Size(Float aWidth, Float aHeight) : Super(aWidth, aHeight) {}
};

struct IntSize :
  public BaseSize<int32_t, IntSize> {
  typedef BaseSize<int32_t, IntSize> Super;

  IntSize() : Super() {}
  IntSize(int32_t aWidth, int32_t aHeight) : Super(aWidth, aHeight) {}
};

}
}

#endif 
