











































#include "nsBoxLayoutState.h"

nsBoxLayoutState::nsBoxLayoutState(nsPresContext* aPresContext,
                                   nsRenderingContext* aRenderingContext,
                                   const nsHTMLReflowState* aOuterReflowState,
                                   PRUint16 aReflowDepth)
  : mPresContext(aPresContext)
  , mRenderingContext(aRenderingContext)
  , mOuterReflowState(aOuterReflowState)
  , mLayoutFlags(0)
  , mReflowDepth(aReflowDepth)
  , mPaintingDisabled(false)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}

nsBoxLayoutState::nsBoxLayoutState(const nsBoxLayoutState& aState)
  : mPresContext(aState.mPresContext)
  , mRenderingContext(aState.mRenderingContext)
  , mOuterReflowState(aState.mOuterReflowState)
  , mLayoutFlags(aState.mLayoutFlags)
  , mReflowDepth(aState.mReflowDepth + 1)
  , mPaintingDisabled(aState.mPaintingDisabled)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}
