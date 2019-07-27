




#include "ScrollbarStyles.h"
#include "nsStyleStruct.h" 

namespace mozilla {

  ScrollbarStyles::ScrollbarStyles(uint8_t aH, uint8_t aV,
                                   const nsStyleDisplay* aDisplay)
    : mHorizontal(aH), mVertical(aV),
      mScrollBehavior(aDisplay->mScrollBehavior),
      mScrollSnapTypeX(aDisplay->mScrollSnapTypeX),
      mScrollSnapTypeY(aDisplay->mScrollSnapTypeY),
      mScrollSnapPointsX(aDisplay->mScrollSnapPointsX),
      mScrollSnapPointsY(aDisplay->mScrollSnapPointsY),
      mScrollSnapDestinationX(aDisplay->mScrollSnapDestination.mXPosition),
      mScrollSnapDestinationY(aDisplay->mScrollSnapDestination.mYPosition) {}

  ScrollbarStyles::ScrollbarStyles(const nsStyleDisplay* aDisplay)
    : mHorizontal(aDisplay->mOverflowX), mVertical(aDisplay->mOverflowY),
      mScrollBehavior(aDisplay->mScrollBehavior),
      mScrollSnapTypeX(aDisplay->mScrollSnapTypeX),
      mScrollSnapTypeY(aDisplay->mScrollSnapTypeY),
      mScrollSnapPointsX(aDisplay->mScrollSnapPointsX),
      mScrollSnapPointsY(aDisplay->mScrollSnapPointsY),
      mScrollSnapDestinationX(aDisplay->mScrollSnapDestination.mXPosition),
      mScrollSnapDestinationY(aDisplay->mScrollSnapDestination.mYPosition) {}

} 
