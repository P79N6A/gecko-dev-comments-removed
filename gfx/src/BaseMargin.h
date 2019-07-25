




































#ifndef MOZILLA_BASEMARGIN_H_
#define MOZILLA_BASEMARGIN_H_

#include "gfxCore.h"

namespace mozilla {





template <class T, class Sub>
struct BaseMargin {
  typedef mozilla::css::Side SideT;

  
  
  T top, right, bottom, left;

  
  BaseMargin() : top(0), right(0), bottom(0), left(0) {}
  BaseMargin(T aLeft, T aTop, T aRight, T aBottom) :
      top(aTop), right(aRight), bottom(aBottom), left(aLeft) {}

  void SizeTo(T aLeft, T aTop, T aRight, T aBottom)
  {
    left = aLeft; top = aTop; right = aRight; bottom = aBottom;
  }

  T LeftRight() const { return left + right; }
  T TopBottom() const { return top + bottom; }

  T& Side(SideT aSide) {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    
    return *(&top + aSide);
  }
  T Side(SideT aSide) const {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    
    return *(&top + aSide);
  }

  
  
  bool operator==(const Sub& aMargin) const {
    return left == aMargin.left && top == aMargin.top &&
           right == aMargin.right && bottom == aMargin.bottom;
  }
  bool operator!=(const Sub& aMargin) const {
    return !(*this == aMargin);
  }
  Sub operator+(const Sub& aMargin) const {
    return Sub(left + aMargin.left, top + aMargin.top,
             right + aMargin.right, bottom + aMargin.bottom);
  }
  Sub operator-(const Sub& aMargin) const {
    return Sub(left - aMargin.left, top - aMargin.top,
             right - aMargin.right, bottom - aMargin.bottom);
  }
  Sub& operator+=(const Sub& aMargin) {
    left += aMargin.left;
    top += aMargin.top;
    right += aMargin.right;
    bottom += aMargin.bottom;
    return *static_cast<Sub*>(this);
  }
};

}

#endif 
