




#ifndef MOZILLA_GFX_POINT_H_
#define MOZILLA_GFX_POINT_H_

#include "Types.h"
#include "BasePoint.h"
#include "BaseSize.h"
#include "nsMathUtils.h"                


namespace mozilla {
namespace gfx {


struct UnknownUnits {};

template<class units>
struct IntPointTyped :
  public BasePoint< int32_t, IntPointTyped<units> >,
  public units {
  typedef BasePoint< int32_t, IntPointTyped<units> > Super;

  IntPointTyped() : Super() {}
  IntPointTyped(int32_t aX, int32_t aY) : Super(aX, aY) {}

  
  

  static IntPointTyped<units> FromUnknownPoint(const IntPointTyped<UnknownUnits>& aPoint) {
    return IntPointTyped<units>(aPoint.x, aPoint.y);
  }

  IntPointTyped<UnknownUnits> ToUnknownPoint() const {
    return IntPointTyped<UnknownUnits>(this->x, this->y);
  }
};
typedef IntPointTyped<UnknownUnits> IntPoint;

template<class units>
struct PointTyped :
  public BasePoint< Float, PointTyped<units> >,
  public units {
  typedef BasePoint< Float, PointTyped<units> > Super;

  PointTyped() : Super() {}
  PointTyped(Float aX, Float aY) : Super(aX, aY) {}
  PointTyped(const IntPointTyped<units>& point) : Super(float(point.x), float(point.y)) {}

  
  

  static PointTyped<units> FromUnknownPoint(const PointTyped<UnknownUnits>& aPoint) {
    return PointTyped<units>(aPoint.x, aPoint.y);
  }

  PointTyped<UnknownUnits> ToUnknownPoint() const {
    return PointTyped<UnknownUnits>(this->x, this->y);
  }
};
typedef PointTyped<UnknownUnits> Point;

template<class units>
IntPointTyped<units> RoundedToInt(const PointTyped<units>& aPoint) {
  return IntPointTyped<units>(NS_lround(aPoint.x),
                              NS_lround(aPoint.y));
}

template<class units>
struct IntSizeTyped :
  public BaseSize< int32_t, IntSizeTyped<units> >,
  public units {
  typedef BaseSize< int32_t, IntSizeTyped<units> > Super;

  IntSizeTyped() : Super() {}
  IntSizeTyped(int32_t aWidth, int32_t aHeight) : Super(aWidth, aHeight) {}

  
  

  static IntSizeTyped<units> FromUnknownSize(const IntSizeTyped<UnknownUnits>& aSize) {
    return IntSizeTyped<units>(aSize.width, aSize.height);
  }

  IntSizeTyped<UnknownUnits> ToUnknownSize() const {
    return IntSizeTyped<UnknownUnits>(this->width, this->height);
  }
};
typedef IntSizeTyped<UnknownUnits> IntSize;

template<class units>
struct SizeTyped :
  public BaseSize< Float, SizeTyped<units> >,
  public units {
  typedef BaseSize< Float, SizeTyped<units> > Super;

  SizeTyped() : Super() {}
  SizeTyped(Float aWidth, Float aHeight) : Super(aWidth, aHeight) {}
  explicit SizeTyped(const IntSizeTyped<units>& size) :
    Super(float(size.width), float(size.height)) {}

  
  

  static SizeTyped<units> FromUnknownSize(const SizeTyped<UnknownUnits>& aSize) {
    return SizeTyped<units>(aSize.width, aSize.height);
  }

  SizeTyped<UnknownUnits> ToUnknownSize() const {
    return SizeTyped<UnknownUnits>(this->width, this->height);
  }
};
typedef SizeTyped<UnknownUnits> Size;

}
}

#endif 
