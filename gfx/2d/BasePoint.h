




#ifndef MOZILLA_GFX_BASEPOINT_H_
#define MOZILLA_GFX_BASEPOINT_H_

#include <cmath>
#include "mozilla/Attributes.h"
#include "mozilla/ToString.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub, class Coord = T>
struct BasePoint {
  T x, y;

  
  MOZ_CONSTEXPR BasePoint() : x(0), y(0) {}
  MOZ_CONSTEXPR BasePoint(Coord aX, Coord aY) : x(aX), y(aY) {}

  void MoveTo(T aX, T aY) { x = aX; y = aY; }
  void MoveBy(T aDx, T aDy) { x += aDx; y += aDy; }

  
  

  bool operator==(const Sub& aPoint) const {
    return x == aPoint.x && y == aPoint.y;
  }
  bool operator!=(const Sub& aPoint) const {
    return x != aPoint.x || y != aPoint.y;
  }

  Sub operator+(const Sub& aPoint) const {
    return Sub(x + aPoint.x, y + aPoint.y);
  }
  Sub operator-(const Sub& aPoint) const {
    return Sub(x - aPoint.x, y - aPoint.y);
  }
  Sub& operator+=(const Sub& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Sub& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    return *static_cast<Sub*>(this);
  }

  Sub operator*(T aScale) const {
    return Sub(x * aScale, y * aScale);
  }
  Sub operator/(T aScale) const {
    return Sub(x / aScale, y / aScale);
  }

  Sub operator-() const {
    return Sub(-x, -y);
  }

  T Length() const {
    return hypot(x, y);
  }

  
  
  
  Sub& Round() {
    x = Coord(floor(T(x) + T(0.5)));
    y = Coord(floor(T(y) + T(0.5)));
    return *static_cast<Sub*>(this);
  }

  friend std::ostream& operator<<(std::ostream& stream, const BasePoint<T, Sub, Coord>& aPoint) {
    return stream << '(' << aPoint.x << ',' << aPoint.y << ')';
  }

};

}
}

#endif 
