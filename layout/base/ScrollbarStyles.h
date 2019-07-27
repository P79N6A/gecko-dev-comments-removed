




#ifndef ScrollbarStyles_h
#define ScrollbarStyles_h

#include <stdint.h>
#include "nsStyleConsts.h"
#include "mozilla/dom/WindowBinding.h"

namespace mozilla {

struct ScrollbarStyles
{
  
  
  uint8_t mHorizontal;
  uint8_t mVertical;
  
  
  uint8_t mScrollBehavior;
  ScrollbarStyles(uint8_t aH, uint8_t aV, uint8_t aB) : mHorizontal(aH),
                                                        mVertical(aV),
                                                        mScrollBehavior(aB) {}
  ScrollbarStyles() {}
  bool operator==(const ScrollbarStyles& aStyles) const {
    return aStyles.mHorizontal == mHorizontal && aStyles.mVertical == mVertical &&
           aStyles.mScrollBehavior == mScrollBehavior;
  }
  bool operator!=(const ScrollbarStyles& aStyles) const {
    return aStyles.mHorizontal != mHorizontal || aStyles.mVertical != mVertical ||
           aStyles.mScrollBehavior != mScrollBehavior;
  }
  bool IsHiddenInBothDirections() const {
    return mHorizontal == NS_STYLE_OVERFLOW_HIDDEN &&
           mVertical == NS_STYLE_OVERFLOW_HIDDEN;
  }
  bool IsSmoothScroll(dom::ScrollBehavior aBehavior) const {
    return aBehavior == dom::ScrollBehavior::Smooth ||
             (aBehavior == dom::ScrollBehavior::Auto &&
               mScrollBehavior == NS_STYLE_SCROLL_BEHAVIOR_SMOOTH);
  }
};

}

#endif
