




#ifndef MOZILLA_BASEPOINT4D_H_
#define MOZILLA_BASEPOINT4D_H_

#include "mozilla/Assertions.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub>
struct BasePoint4D {
  T x, y, z, w;

  
  BasePoint4D() : x(0), y(0), z(0), w(0) {}
  BasePoint4D(T aX, T aY, T aZ, T aW) : x(aX), y(aY), z(aZ), w(aW) {}

  void MoveTo(T aX, T aY, T aZ, T aW) { x = aX; y = aY; z = aZ; w = aW; }
  void MoveBy(T aDx, T aDy, T aDz, T aDw) { x += aDx; y += aDy; z += aDz; w += aDw; }

  
  

  bool operator==(const Sub& aPoint) const {
    return x == aPoint.x && y == aPoint.y && 
           z == aPoint.z && w == aPoint.w;
  }
  bool operator!=(const Sub& aPoint) const {
    return x != aPoint.x || y != aPoint.y || 
           z != aPoint.z || w != aPoint.w;
  }

  Sub operator+(const Sub& aPoint) const {
    return Sub(x + aPoint.x, y + aPoint.y, z + aPoint.z, w + aPoint.w);
  }
  Sub operator-(const Sub& aPoint) const {
    return Sub(x - aPoint.x, y - aPoint.y, z - aPoint.z, w - aPoint.w);
  }
  Sub& operator+=(const Sub& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    z += aPoint.z;
    w += aPoint.w;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Sub& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    z -= aPoint.z;
    w -= aPoint.w;
    return *static_cast<Sub*>(this);
  }

  Sub operator*(T aScale) const {
    return Sub(x * aScale, y * aScale, z * aScale, w * aScale);
  }
  Sub operator/(T aScale) const {
    return Sub(x / aScale, y / aScale, z / aScale, w / aScale);
  }

  Sub& operator*=(T aScale) {
    x *= aScale;
    y *= aScale;
    z *= aScale;
    w *= aScale;
    return *static_cast<Sub*>(this);
  }

  Sub& operator/=(T aScale) {
    x /= aScale;
    y /= aScale;
    z /= aScale;
    w /= aScale;
    return *static_cast<Sub*>(this);
  }

  Sub operator-() const {
    return Sub(-x, -y, -z, -w);
  }

  T& operator[](int aIndex) {
    MOZ_ASSERT(aIndex >= 0 && aIndex <= 3, "Invalid array index");
    return *((&x)+aIndex);
  }

  const T& operator[](int aIndex) const {
    MOZ_ASSERT(aIndex >= 0 && aIndex <= 3, "Invalid array index");
    return *((&x)+aIndex);
  }

  T DotProduct(const Sub& aPoint) const {
    return x * aPoint.x + y * aPoint.y + z * aPoint.z + w * aPoint.w;
  }

  
  Sub CrossProduct(const Sub& aPoint) const {
      return Sub(y * aPoint.z - aPoint.y * z,
          z * aPoint.x - aPoint.z * x,
          x * aPoint.y - aPoint.x * y, 
          0);
  }

  T Length() const {
    return sqrt(x*x + y*y + z*z + w*w);
  }

  void Normalize() {
    *this /= Length();
  }

  bool HasPositiveWCoord() { return w > 0; }
};

}
}

#endif 
