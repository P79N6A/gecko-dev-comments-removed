




#ifndef MOZILLA_GFX_POINT_H_
#define MOZILLA_GFX_POINT_H_

#include "mozilla/Attributes.h"
#include "Types.h"
#include "Coord.h"
#include "BaseCoord.h"
#include "BasePoint.h"
#include "BasePoint3D.h"
#include "BasePoint4D.h"
#include "BaseSize.h"
#include "mozilla/TypeTraits.h"

#include <cmath>

namespace mozilla {

template <typename> struct IsPixel;

namespace gfx {


struct UnknownUnits {};

}  

template<> struct IsPixel<gfx::UnknownUnits> : TrueType {};

namespace gfx {

template<class units>
struct IntPointTyped :
  public BasePoint< int32_t, IntPointTyped<units>, IntCoordTyped<units> >,
  public units {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef IntCoordTyped<units> Coord;
  typedef BasePoint< int32_t, IntPointTyped<units>, IntCoordTyped<units> > Super;

  MOZ_CONSTEXPR IntPointTyped() : Super() {}
  MOZ_CONSTEXPR IntPointTyped(int32_t aX, int32_t aY) : Super(Coord(aX), Coord(aY)) {}
  
  
  MOZ_CONSTEXPR IntPointTyped(int32_t aX, Coord aY) : Super(Coord(aX), aY) {}
  MOZ_CONSTEXPR IntPointTyped(Coord aX, int32_t aY) : Super(aX, Coord(aY)) {}
  MOZ_CONSTEXPR IntPointTyped(Coord aX, Coord aY) : Super(aX, aY) {}

  
  

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
  public BasePoint< Float, PointTyped<units>, CoordTyped<units> >,
  public units {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef CoordTyped<units> Coord;
  typedef BasePoint< Float, PointTyped<units>, CoordTyped<units> > Super;

  MOZ_CONSTEXPR PointTyped() : Super() {}
  MOZ_CONSTEXPR PointTyped(Float aX, Float aY) : Super(Coord(aX), Coord(aY)) {}
  
  
  MOZ_CONSTEXPR PointTyped(Float aX, Coord aY) : Super(Coord(aX), aY) {}
  MOZ_CONSTEXPR PointTyped(Coord aX, Float aY) : Super(aX, Coord(aY)) {}
  MOZ_CONSTEXPR PointTyped(Coord aX, Coord aY) : Super(aX.value, aY.value) {}
  MOZ_CONSTEXPR MOZ_IMPLICIT PointTyped(const IntPointTyped<units>& point) : Super(float(point.x), float(point.y)) {}

  
  

  static PointTyped<units> FromUnknownPoint(const PointTyped<UnknownUnits>& aPoint) {
    return PointTyped<units>(aPoint.x, aPoint.y);
  }

  PointTyped<UnknownUnits> ToUnknownPoint() const {
    return PointTyped<UnknownUnits>(this->x, this->y);
  }
};
typedef PointTyped<UnknownUnits> Point;
static_assert(sizeof(Point) == 2*sizeof(Float));

template<class units>
IntPointTyped<units> RoundedToInt(const PointTyped<units>& aPoint) {
  return IntPointTyped<units>(int32_t(floorf(aPoint.x + 0.5f)),
                              int32_t(floorf(aPoint.y + 0.5f)));
}

template<class units>
IntPointTyped<units> TruncatedToInt(const PointTyped<units>& aPoint) {
  return IntPointTyped<units>(int32_t(aPoint.x),
                              int32_t(aPoint.y));
}

template<class units>
struct Point3DTyped :
  public BasePoint3D< Float, Point3DTyped<units> > {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BasePoint3D< Float, Point3DTyped<units> > Super;

  Point3DTyped() : Super() {}
  Point3DTyped(Float aX, Float aY, Float aZ) : Super(aX, aY, aZ) {}

  
  

  static Point3DTyped<units> FromUnknownPoint(const Point3DTyped<UnknownUnits>& aPoint) {
    return Point3DTyped<units>(aPoint.x, aPoint.y, aPoint.z);
  }

  Point3DTyped<UnknownUnits> ToUnknownPoint() const {
    return Point3DTyped<UnknownUnits>(this->x, this->y, this->z);
  }
};
typedef Point3DTyped<UnknownUnits> Point3D;

template<class units>
struct Point4DTyped :
  public BasePoint4D< Float, Point4DTyped<units> > {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BasePoint4D< Float, Point4DTyped<units> > Super;

  Point4DTyped() : Super() {}
  Point4DTyped(Float aX, Float aY, Float aZ, Float aW) : Super(aX, aY, aZ, aW) {}

  
  

  static Point4DTyped<units> FromUnknownPoint(const Point4DTyped<UnknownUnits>& aPoint) {
    return Point4DTyped<units>(aPoint.x, aPoint.y, aPoint.z, aPoint.w);
  }

  Point4DTyped<UnknownUnits> ToUnknownPoint() const {
    return Point4DTyped<UnknownUnits>(this->x, this->y, this->z, this->w);
  }

  PointTyped<units> As2DPoint() {
    return PointTyped<units>(this->x / this->w, this->y / this->w);
  }
};
typedef Point4DTyped<UnknownUnits> Point4D;

template<class units>
struct IntSizeTyped :
  public BaseSize< int32_t, IntSizeTyped<units> >,
  public units {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BaseSize< int32_t, IntSizeTyped<units> > Super;

  MOZ_CONSTEXPR IntSizeTyped() : Super() {}
  MOZ_CONSTEXPR IntSizeTyped(int32_t aWidth, int32_t aHeight) : Super(aWidth, aHeight) {}

  
  

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
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BaseSize< Float, SizeTyped<units> > Super;

  MOZ_CONSTEXPR SizeTyped() : Super() {}
  MOZ_CONSTEXPR SizeTyped(Float aWidth, Float aHeight) : Super(aWidth, aHeight) {}
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

template<class units>
IntSizeTyped<units> RoundedToInt(const SizeTyped<units>& aSize) {
  return IntSizeTyped<units>(int32_t(floorf(aSize.width + 0.5f)),
                             int32_t(floorf(aSize.height + 0.5f)));
}

}
}

#endif 
