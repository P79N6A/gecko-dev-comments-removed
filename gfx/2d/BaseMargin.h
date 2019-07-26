




#ifndef MOZILLA_GFX_BASEMARGIN_H_
#define MOZILLA_GFX_BASEMARGIN_H_

#include "Types.h"

namespace mozilla {
namespace gfx {





template <class T, class Sub>
struct BaseMargin {
  typedef mozilla::css::Side SideT;

  
  
  T top, right, bottom, left;

  
  BaseMargin() : top(0), right(0), bottom(0), left(0) {}
  BaseMargin(T aTop, T aRight, T aBottom, T aLeft) :
      top(aTop), right(aRight), bottom(aBottom), left(aLeft) {}

  void SizeTo(T aTop, T aRight, T aBottom, T aLeft)
  {
    top = aTop; right = aRight; bottom = aBottom; left = aLeft;
  }

  T LeftRight() const { return left + right; }
  T TopBottom() const { return top + bottom; }

  T& Side(SideT aSide) {
    
    return *(&top + T(aSide));
  }
  T Side(SideT aSide) const {
    
    return *(&top + T(aSide));
  }

  void ApplySkipSides(int aSkipSides)
  {
    if (aSkipSides & (1 << mozilla::css::eSideTop)) {
      top = 0;
    }
    if (aSkipSides & (1 << mozilla::css::eSideRight)) {
      right = 0;
    }
    if (aSkipSides & (1 << mozilla::css::eSideBottom)) {
      bottom = 0;
    }
    if (aSkipSides & (1 << mozilla::css::eSideLeft)) {
      left = 0;
    }
  }

  
  
  bool operator==(const Sub& aMargin) const {
    return top == aMargin.top && right == aMargin.right &&
           bottom == aMargin.bottom && left == aMargin.left;
  }
  bool operator!=(const Sub& aMargin) const {
    return !(*this == aMargin);
  }
  Sub operator+(const Sub& aMargin) const {
    return Sub(top + aMargin.top, right + aMargin.right,
               bottom + aMargin.bottom, left + aMargin.left);
  }
  Sub operator-(const Sub& aMargin) const {
    return Sub(top - aMargin.top, right - aMargin.right,
               bottom - aMargin.bottom, left - aMargin.left);
  }
  Sub& operator+=(const Sub& aMargin) {
    top += aMargin.top;
    right += aMargin.right;
    bottom += aMargin.bottom;
    left += aMargin.left;
    return *static_cast<Sub*>(this);
  }
};

}
}

#endif 
