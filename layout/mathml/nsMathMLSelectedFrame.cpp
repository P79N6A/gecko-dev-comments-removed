




#include "nsMathMLSelectedFrame.h"
#include "nsDisplayList.h"

using namespace mozilla;

nsMathMLSelectedFrame::~nsMathMLSelectedFrame()
{
}

void
nsMathMLSelectedFrame::Init(nsIContent*       aContent,
                            nsContainerFrame* aParent,
                            nsIFrame*         aPrevInFlow)
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

void
nsMathMLSelectedFrame::SetInitialChildList(ChildListID     aListID,
                                           nsFrameList&    aChildList)
{
  nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);
  
  
  GetSelectedFrame();
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


LogicalSize
nsMathMLSelectedFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                   WritingMode aWM,
                                   const LogicalSize& aCBSize,
                                   nscoord aAvailableISize,
                                   const LogicalSize& aMargin,
                                   const LogicalSize& aBorder,
                                   const LogicalSize& aPadding,
                                   ComputeSizeFlags aFlags)
{
  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    
    
    
    nscoord availableISize = aAvailableISize - aBorder.ISize(aWM) -
        aPadding.ISize(aWM) - aMargin.ISize(aWM);
    LogicalSize cbSize = aCBSize - aBorder - aPadding - aMargin;
    nsCSSOffsetState offsetState(childFrame, aRenderingContext, availableISize);
    LogicalSize size =
        childFrame->ComputeSize(aRenderingContext, aWM, cbSize,
            availableISize, offsetState.ComputedLogicalMargin().Size(aWM),
            offsetState.ComputedLogicalBorderPadding().Size(aWM) -
            offsetState.ComputedLogicalPadding().Size(aWM),
            offsetState.ComputedLogicalPadding().Size(aWM),
            aFlags);
    return size + offsetState.ComputedLogicalBorderPadding().Size(aWM);
  }
  return LogicalSize(aWM);
}


void
nsMathMLSelectedFrame::Reflow(nsPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  MarkInReflow();
  mPresentationData.flags &= ~NS_MATHML_ERROR;
  aStatus = NS_FRAME_COMPLETE;
  aDesiredSize.ClearSize();
  aDesiredSize.SetBlockStartAscent(0);
  mBoundingMetrics = nsBoundingMetrics();
  nsIFrame* childFrame = GetSelectedFrame();
  if (childFrame) {
    WritingMode wm = childFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    ReflowChild(childFrame, aPresContext, aDesiredSize,
                childReflowState, aStatus);
    SaveReflowAndBoundingMetricsFor(childFrame, aDesiredSize,
                                    aDesiredSize.mBoundingMetrics);
    mBoundingMetrics = aDesiredSize.mBoundingMetrics;
  }
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
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

  aDesiredSize.ClearSize();
  aDesiredSize.SetBlockStartAscent(0);
  mBoundingMetrics = nsBoundingMetrics();
  if (childFrame) {
    GetReflowAndBoundingMetricsFor(childFrame, aDesiredSize, mBoundingMetrics);
    if (aPlaceOrigin) {
      FinishReflowChild(childFrame, PresContext(), aDesiredSize, nullptr, 0, 0, 0);
    }
    mReference.x = 0;
    mReference.y = aDesiredSize.BlockStartAscent();
  }
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  return NS_OK;
}
