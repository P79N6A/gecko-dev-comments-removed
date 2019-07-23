











































#include "nsBoxLayoutState.h"

nsBoxLayoutState::nsBoxLayoutState(nsPresContext* aPresContext,
                                   nsIRenderingContext* aRenderingContext)
  : mPresContext(aPresContext)
  , mRenderingContext(aRenderingContext)
  , mLayoutFlags(0)
  , mPaintingDisabled(PR_FALSE)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}

nsBoxLayoutState::nsBoxLayoutState(const nsBoxLayoutState& aState)
  : mPresContext(aState.mPresContext)
  , mRenderingContext(aState.mRenderingContext)
  , mLayoutFlags(aState.mLayoutFlags)
  , mPaintingDisabled(aState.mPaintingDisabled)
{
  NS_ASSERTION(mPresContext, "PresContext must be non-null");
}
