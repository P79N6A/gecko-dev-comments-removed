




#ifndef ScrollbarStyles_h
#define ScrollbarStyles_h

#include <stdint.h>
#include "nsStyleConsts.h" 
#include "nsStyleCoord.h" 
#include "mozilla/dom/WindowBinding.h"


struct nsStyleDisplay;

namespace mozilla {

struct ScrollbarStyles
{
  
  
  uint8_t mHorizontal;
  uint8_t mVertical;
  
  
  uint8_t mScrollBehavior;
  
  
  uint8_t mScrollSnapTypeX;
  uint8_t mScrollSnapTypeY;
  nsStyleCoord mScrollSnapPointsX;
  nsStyleCoord mScrollSnapPointsY;
  nsStyleCoord::CalcValue mScrollSnapDestinationX;
  nsStyleCoord::CalcValue mScrollSnapDestinationY;

  ScrollbarStyles(uint8_t aH, uint8_t aV)
    : mHorizontal(aH), mVertical(aV),
      mScrollBehavior(NS_STYLE_SCROLL_BEHAVIOR_AUTO),
      mScrollSnapTypeX(NS_STYLE_SCROLL_SNAP_TYPE_NONE),
      mScrollSnapTypeY(NS_STYLE_SCROLL_SNAP_TYPE_NONE),
      mScrollSnapPointsX(nsStyleCoord(eStyleUnit_None)),
      mScrollSnapPointsY(nsStyleCoord(eStyleUnit_None)) {

    mScrollSnapDestinationX.mPercent = 0;
    mScrollSnapDestinationX.mLength = nscoord(0.0f);
    mScrollSnapDestinationX.mHasPercent = false;
    mScrollSnapDestinationY.mPercent = 0;
    mScrollSnapDestinationY.mLength = nscoord(0.0f);
    mScrollSnapDestinationY.mHasPercent = false;
  }

  explicit ScrollbarStyles(const nsStyleDisplay* aDisplay);
  ScrollbarStyles(uint8_t aH, uint8_t aV, const nsStyleDisplay* aDisplay);
  ScrollbarStyles() {}
  bool operator==(const ScrollbarStyles& aStyles) const {
    return aStyles.mHorizontal == mHorizontal && aStyles.mVertical == mVertical &&
           aStyles.mScrollBehavior == mScrollBehavior &&
           aStyles.mScrollSnapTypeX == mScrollSnapTypeX &&
           aStyles.mScrollSnapTypeY == mScrollSnapTypeY &&
           aStyles.mScrollSnapPointsX == mScrollSnapPointsX &&
           aStyles.mScrollSnapPointsY == mScrollSnapPointsY &&
           aStyles.mScrollSnapDestinationX == mScrollSnapDestinationX &&
           aStyles.mScrollSnapDestinationY == mScrollSnapDestinationY;
  }
  bool operator!=(const ScrollbarStyles& aStyles) const {
    return !(*this == aStyles);
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
