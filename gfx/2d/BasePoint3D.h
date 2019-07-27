




#ifndef MOZILLA_BASEPOINT3D_H_
#define MOZILLA_BASEPOINT3D_H_

#include "mozilla/Assertions.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub>
struct BasePoint3D {
  T x, y, z;

  
  BasePoint3D() : x(0), y(0), z(0) {}
  BasePoint3D(T aX, T aY, T aZ) : x(aX), y(aY), z(aZ) {}

  void MoveTo(T aX, T aY, T aZ) { x = aX; y = aY; z = aZ; }
  void MoveBy(T aDx, T aDy, T aDz) { x += aDx; y += aDy; z += aDz; }

  
  

  T& operator[](int aIndex) {
    MOZ_ASSERT(aIndex >= 0 && aIndex <= 2);
    return *((&x)+aIndex);
  }

  const T& operator[](int aIndex) const {
    MOZ_ASSERT(aIndex >= 0 && aIndex <= 2);
    return *((&x)+aIndex);
  }

  bool operator==(const Sub& aPoint) const {
    return x == aPoint.x && y == aPoint.y && z == aPoint.z;
  }
  bool operator!=(const Sub& aPoint) const {
    return x != aPoint.x || y != aPoint.y || z != aPoint.z;
  }

  Sub operator+(const Sub& aPoint) const {
    return Sub(x + aPoint.x, y + aPoint.y, z + aPoint.z);
  }
  Sub operator-(const Sub& aPoint) const {
    return Sub(x - aPoint.x, y - aPoint.y, z - aPoint.z);
  }
  Sub& operator+=(const Sub& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    z += aPoint.z;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Sub& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    z -= aPoint.z;
    return *static_cast<Sub*>(this);
  }

  Sub operator*(T aScale) const {
    return Sub(x * aScale, y * aScale, z * aScale);
  }
  Sub operator/(T aScale) const {
    return Sub(x / aScale, y / aScale, z / aScale);
  }

  Sub& operator*=(T aScale) {
    x *= aScale;
    y *= aScale;
    z *= aScale;
    return *static_cast<Sub*>(this);
  }

  Sub& operator/=(T aScale) {
      x /= aScale;
      y /= aScale;
      z /= aScale;
      return *static_cast<Sub*>(this);
  }

  Sub operator-() const {
    return Sub(-x, -y, -z);
  }

  Sub CrossProduct(const Sub& aPoint) const {
      return Sub(y * aPoint.z - aPoint.y * z,
                 z * aPoint.x - aPoint.z * x,
                 x * aPoint.y - aPoint.x * y);
  }

  T DotProduct(const Sub& aPoint) const {
      return x * aPoint.x + y * aPoint.y + z * aPoint.z;
  }

  T Length() const {
      return sqrt(x*x + y*y + z*z);
  }

  
  void Normalize() {
      *this /= Length();
  }
};

} 
} 

#endif 
