




































#ifndef MOZILLA_BASESIZE_H_
#define MOZILLA_BASESIZE_H_

namespace mozilla {






template <class T, class Sub>
struct BaseSize {
  T width, height;

  
  BaseSize() : width(0), height(0) {}
  BaseSize(T aWidth, T aHeight) : width(aWidth), height(aHeight) {}

  void SizeTo(T aWidth, T aHeight) { width = aWidth; height = aHeight; }

  
  

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
};

}

#endif 
