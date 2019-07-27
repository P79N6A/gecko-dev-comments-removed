




#ifndef MOZILLA_GFX_BASECOORD_H_
#define MOZILLA_GFX_BASECOORD_H_

#include "mozilla/Attributes.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub>
struct BaseCoord {
  T value;

  
  MOZ_CONSTEXPR BaseCoord() : value(0) {}
  explicit MOZ_CONSTEXPR BaseCoord(T aValue) : value(aValue) {}

  
  

  operator T() const { return value; }

  friend bool operator==(Sub aA, Sub aB) {
    return aA.value == aB.value;
  }
  friend bool operator!=(Sub aA, Sub aB) {
    return aA.value != aB.value;
  }

  friend Sub operator+(Sub aA, Sub aB) {
    return Sub(aA.value + aB.value);
  }
  friend Sub operator-(Sub aA, Sub aB) {
    return Sub(aA.value - aB.value);
  }
  friend Sub operator*(Sub aCoord, T aScale) {
    return Sub(aCoord.value * aScale);
  }
  friend Sub operator*(T aScale, Sub aCoord) {
    return Sub(aScale * aCoord.value);
  }
  friend Sub operator/(Sub aCoord, T aScale) {
    return Sub(aCoord.value / aScale);
  }
  

  Sub& operator+=(Sub aCoord) {
    value += aCoord.value;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(Sub aCoord) {
    value -= aCoord.value;
    return *static_cast<Sub*>(this);
  }
  Sub& operator*=(T aScale) {
    value *= aScale;
    return *static_cast<Sub*>(this);
  }
  Sub& operator/=(T aScale) {
    value /= aScale;
    return *static_cast<Sub*>(this);
  }

  
  
  
  
  friend bool operator==(Sub aA, T aB) {
    return aA.value == aB;
  }
  friend bool operator==(T aA, Sub aB) {
    return aA == aB.value;
  }
  friend bool operator!=(Sub aA, T aB) {
    return aA.value != aB;
  }
  friend bool operator!=(T aA, Sub aB) {
    return aA != aB.value;
  }
  friend T operator+(Sub aA, T aB) {
    return aA.value + aB;
  }
  friend T operator+(T aA, Sub aB) {
    return aA + aB.value;
  }
  friend T operator-(Sub aA, T aB) {
    return aA.value - aB;
  }
  friend T operator-(T aA, Sub aB) {
    return aA - aB.value;
  }

  Sub operator-() const {
    return Sub(-value);
  }
};

}
}

#endif 
