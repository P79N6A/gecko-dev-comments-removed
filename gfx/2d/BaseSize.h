




#ifndef MOZILLA_GFX_BASESIZE_H_
#define MOZILLA_GFX_BASESIZE_H_

#include "mozilla/Attributes.h"

namespace mozilla {
namespace gfx {






template <class T, class Sub>
struct BaseSize {
  T width, height;

  
  MOZ_CONSTEXPR BaseSize() : width(0), height(0) {}
  MOZ_CONSTEXPR BaseSize(T aWidth, T aHeight) : width(aWidth), height(aHeight) {}

  void SizeTo(T aWidth, T aHeight) { width = aWidth; height = aHeight; }

  bool IsEmpty() const {
    return width == 0 || height == 0;
  }

  
  

  bool operator==(const Sub& aSize) const {
    return width == aSize.width && height == aSize.height;
  }
  bool operator!=(const Sub& aSize) const {
    return width != aSize.width || height != aSize.height;
  }
  bool operator<=(const Sub& aSize) const {
    return width <= aSize.width && height <= aSize.height;
  }
  bool operator<(const Sub& aSize) const {
    return *this <= aSize && *this != aSize;
  }

  Sub operator+(const Sub& aSize) const {
    return Sub(width + aSize.width, height + aSize.height);
  }
  Sub operator-(const Sub& aSize) const {
    return Sub(width - aSize.width, height - aSize.height);
  }
  Sub& operator+=(const Sub& aSize) {
    width += aSize.width;
    height += aSize.height;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Sub& aSize) {
    width -= aSize.width;
    height -= aSize.height;
    return *static_cast<Sub*>(this);
  }

  Sub operator*(T aScale) const {
    return Sub(width * aScale, height * aScale);
  }
  Sub operator/(T aScale) const {
    return Sub(width / aScale, height / aScale);
  }
  void Scale(T aXScale, T aYScale) {
    width *= aXScale;
    height *= aYScale;
  }

  Sub operator*(const Sub& aSize) const {
    return Sub(width * aSize.width, height * aSize.height);
  }
  Sub operator/(const Sub& aSize) const {
    return Sub(width / aSize.width, height / aSize.height);
  }
};

}
}

#endif 
