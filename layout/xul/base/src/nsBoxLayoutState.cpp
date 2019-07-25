











































#include "nsBoxLayoutState.h"

nsBoxLayoutState::nsBoxLayoutState(nsPresContext* aPresContext,
                                   nsRenderingContext* aRenderingContext,
                                   PRUint16 aReflowDepth)
  : mPresContext(aPresContext)
  , mRenderingContext(aRenderingContext)
  , mLayoutFlags(0)
  , mReflowDepth(aReflowDepth)
  , mPaintingDisabled(PR_FALSE)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}

nsBoxLayoutState::nsBoxLayoutState(const nsBoxLayoutState& aState)
  : mPresContext(aState.mPresContext)
  , mRenderingContext(aState.mRenderingContext)
  , mLayoutFlags(aState.mLayoutFlags)
  , mReflowDepth(aState.mReflowDepth + 1)
  , mPaintingDisabled(aState.mPaintingDisabled)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}
