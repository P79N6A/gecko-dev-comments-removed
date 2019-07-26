






#include "nsHTMLReflowMetrics.h"
#include "nsHTMLReflowState.h"

void
nsOverflowAreas::UnionWith(const nsOverflowAreas& aOther)
{
  
  
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    mRects[otype].UnionRect(mRects[otype], aOther.mRects[otype]);
  }
}

void
nsOverflowAreas::UnionAllWith(const nsRect& aRect)
{
  
  
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    mRects[otype].UnionRect(mRects[otype], aRect);
  }
}

void
nsOverflowAreas::SetAllTo(const nsRect& aRect)
{
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    mRects[otype] = aRect;
  }
}

nsHTMLReflowMetrics::nsHTMLReflowMetrics(const nsHTMLReflowState& aState,
                                         uint32_t aFlags)
  : mISize(0)
  , mBSize(0)
  , mBlockStartAscent(ASK_FOR_BASELINE)
  , mFlags(aFlags)
  , mWritingMode(aState.GetWritingMode())
{
}

void
nsHTMLReflowMetrics::SetOverflowAreasToDesiredBounds()
{
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    mOverflowAreas.Overflow(otype).SetRect(0, 0, Width(), Height());
  }
}

void
nsHTMLReflowMetrics::UnionOverflowAreasWithDesiredBounds()
{
  
  
  nsRect rect(0, 0, Width(), Height());
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    nsRect& o = mOverflowAreas.Overflow(otype);
    o.UnionRect(o, rect);
  }
}
