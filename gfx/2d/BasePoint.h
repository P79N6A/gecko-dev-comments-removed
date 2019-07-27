




#ifndef MOZILLA_GFX_BASEPOINT_H_
#define MOZILLA_GFX_BASEPOINT_H_

#include <cmath>
#include "mozilla/Attributes.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub>
struct BasePoint {
  T x, y;

  
  MOZ_CONSTEXPR BasePoint() : x(0), y(0) {}
  MOZ_CONSTEXPR BasePoint(T aX, T aY) : x(aX), y(aY) {}

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
    x = static_cast<T>(floor(x + 0.5));
    y = static_cast<T>(floor(y + 0.5));
    return *static_cast<Sub*>(this);
  }

};

}
}

#endif 
