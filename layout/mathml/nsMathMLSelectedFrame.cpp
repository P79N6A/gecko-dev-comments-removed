




#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsMathMLSelectedFrame.h"
#include "nsDisplayList.h"

nsMathMLSelectedFrame::~nsMathMLSelectedFrame()
{
}

void
nsMathMLSelectedFrame::Init(nsIContent*      aContent,
                            nsIFrame*        aParent,
                            nsIFrame*        aPrevInFlow)
{
  
  mInvalidMarkup = false;
  mSelectedFrame = nullptr;

  
  nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

NS_IMETHODIMP
nsMathMLSelectedFrame::TransmitAutomaticData()
{
  
  
  

  
  
  
  nsIMathMLFrame* mathMLFrame = do_QueryFrame(mSelectedFrame);
  if (mathMLFrame && mathMLFrame->IsSpaceLike()) {
    mPresentationData.flags |= NS_MATHML_SPACE_LIKE;
  } else {
    mPresentationData.flags &= ~NS_MATHML_SPACE_LIKE;
  }

  
  
  
  mPresentationData.baseFrame = mSelectedFrame;
  GetEmbellishDataFrom(mSelectedFrame, mEmbellishData);

  return NS_OK;
}

nsresult
nsMathMLSelectedFrame::ChildListChanged(int32_t aModType)
{
  GetSelectedFrame();
  return nsMathMLContainerFrame::ChildListChanged(aModType);
}

NS_IMETHODIMP
nsMathMLSelectedFrame::SetInitialChildList(ChildListID     aListID,
                                           nsFrameList&    aChildList)
{
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListID,
                                                            aChildList);
  
  
  GetSelectedFrame();
  return rv;
}


void
nsMathMLSelectedFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  
  
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
    return;
  }

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    
    nsDisplayListSet set(aLists, aLists.Content());
    
    BuildDisplayListForChild(aBuilder, childFrame, aDirtyRect, set);
  }

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
  
  DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
}


NS_IMETHODIMP
nsMathMLSelectedFrame::Reflow(nsPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics = nsBoundingMetrics();
  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, aDesiredSize,
                     childReflowState, aStatus);
    SaveReflowAndBoundingMetricsFor(childFrame, aDesiredSize,
                                    aDesiredSize.mBoundingMetrics);
    mBoundingMetrics = aDesiredSize.mBoundingMetrics;
  }
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}


 nsresult
nsMathMLSelectedFrame::Place(nsRenderingContext& aRenderingContext,
                             bool                 aPlaceOrigin,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  nsIFrame* childFrame = GetSelectedFrame();

  if (mInvalidMarkup) {
    return ReflowError(aRenderingContext, aDesiredSize);
  }

  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics = nsBoundingMetrics();
  if (childFrame) {
    GetReflowAndBoundingMetricsFor(childFrame, aDesiredSize, mBoundingMetrics);
    if (aPlaceOrigin) {
      FinishReflowChild(childFrame, PresContext(), nullptr, aDesiredSize, 0, 0, 0);
    }
    mReference.x = 0;
    mReference.y = aDesiredSize.ascent;
  }
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  return NS_OK;
}
