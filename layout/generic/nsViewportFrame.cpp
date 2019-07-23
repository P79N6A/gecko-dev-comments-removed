









































#include "nsCOMPtr.h"
#include "nsViewportFrame.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewViewportFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) ViewportFrame(aContext);
}

NS_IMETHODIMP
ViewportFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  return Super::Init(aContent, aParent, aPrevInFlow);
}

void
ViewportFrame::Destroy()
{
  mFixedContainer.DestroyFrames(this);
  nsContainerFrame::Destroy();
}

NS_IMETHODIMP
ViewportFrame::SetInitialChildList(nsIAtom*        aListName,
                                   nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;

  
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  if (mFixedContainer.GetChildListName() == aListName) {
    rv = mFixedContainer.SetInitialChildList(this, aListName, aChildList);
  } 
  else {
    rv = nsContainerFrame::SetInitialChildList(aListName, aChildList);
  }

  return rv;
}

NS_IMETHODIMP
ViewportFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  
  
  
  
  aBuilder->MarkFramesForDisplayList(this, mFixedContainer.GetFirstChild(), aDirtyRect);

  nsIFrame* kid = mFrames.FirstChild();
  if (!kid)
    return NS_OK;

  
  
  
  return BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
}

NS_IMETHODIMP
ViewportFrame::AppendFrames(nsIAtom*        aListName,
                            nsIFrame*       aFrameList)
{
  nsresult rv = NS_OK;

  if (mFixedContainer.GetChildListName() == aListName) {
    rv = mFixedContainer.AppendFrames(this, aListName, aFrameList);
  }
  else {
    
    NS_ERROR("unexpected child list");
    rv = NS_ERROR_INVALID_ARG;
  }

  return rv;
}

NS_IMETHODIMP
ViewportFrame::InsertFrames(nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsIFrame*       aFrameList)
{
  nsresult rv = NS_OK;

  if (mFixedContainer.GetChildListName() == aListName) {
    rv = mFixedContainer.InsertFrames(this, aListName, aPrevFrame, aFrameList);
  }
  else {
    
    NS_ERROR("unexpected child list");
    rv = NS_ERROR_INVALID_ARG;
  }

  return rv;
}

NS_IMETHODIMP
ViewportFrame::RemoveFrame(nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = NS_OK;

  if (mFixedContainer.GetChildListName() == aListName) {
    rv = mFixedContainer.RemoveFrame(this, aListName, aOldFrame);
  }
  else {
    
    NS_ERROR("unexpected child list");
    rv = NS_ERROR_INVALID_ARG;
  }

  return rv;
}

nsIAtom*
ViewportFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "illegal index");

  if (0 == aIndex) {
    return mFixedContainer.GetChildListName();
  }

  return nsnull;
}

nsIFrame*
ViewportFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (mFixedContainer.GetChildListName() == aListName) {
    nsIFrame* result = nsnull;
    mFixedContainer.FirstChild(this, aListName, &result);
    return result;
  }

  return nsContainerFrame::GetFirstChild(aListName);
}

 nscoord
ViewportFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetMinWidth(aRenderingContext);
    
  

  return result;
}

 nscoord
ViewportFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetPrefWidth(aRenderingContext);
    
  

  return result;
}

nsPoint
 ViewportFrame::AdjustReflowStateForScrollbars(nsHTMLReflowState* aReflowState) const
{
  
  
  

  
  nsIFrame* kidFrame = mFrames.FirstChild();
  nsCOMPtr<nsIScrollableFrame> scrollingFrame(do_QueryInterface(kidFrame));

  if (scrollingFrame) {
    nsMargin scrollbars = scrollingFrame->GetActualScrollbarSizes();
    aReflowState->SetComputedWidth(aReflowState->ComputedWidth() -
                                   scrollbars.LeftRight());
    aReflowState->availableWidth -= scrollbars.LeftRight();
    aReflowState->SetComputedHeight(aReflowState->ComputedHeight() -
                                    scrollbars.TopBottom());
    
    return nsPoint(scrollbars.left, scrollbars.top);
  }
  return nsPoint(0, 0);
}

NS_IMETHODIMP
ViewportFrame::Reflow(nsPresContext*          aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("ViewportFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("ViewportFrame::Reflow");

  
  aStatus = NS_FRAME_COMPLETE;

  
  
  AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  
  
  
  
  nsRect kidRect(0,0,aReflowState.availableWidth,aReflowState.availableHeight);

  nsresult rv = NS_OK;
  
  if (mFrames.NotEmpty()) {
    
    
    if (aReflowState.ShouldReflowAllKids() ||
        aReflowState.mFlags.mVResize ||
        NS_SUBTREE_DIRTY(mFrames.FirstChild())) {
      
      nsIFrame*           kidFrame = mFrames.FirstChild();
      nsHTMLReflowMetrics kidDesiredSize;
      nsSize              availableSpace(aReflowState.availableWidth,
                                         aReflowState.availableHeight);
      nsHTMLReflowState   kidReflowState(aPresContext, aReflowState,
                                         kidFrame, availableSpace);

      
      kidReflowState.SetComputedHeight(aReflowState.availableHeight);
      rv = ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                       0, 0, 0, aStatus);
      kidRect.width = kidDesiredSize.width;
      kidRect.height = kidDesiredSize.height;

      FinishReflowChild(kidFrame, aPresContext, nsnull, kidDesiredSize, 0, 0, 0);
    }
  }

  NS_ASSERTION(aReflowState.availableWidth != NS_UNCONSTRAINEDSIZE,
               "shouldn't happen anymore");

  
  aDesiredSize.width = aReflowState.availableWidth;
  
  
  aDesiredSize.height = aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE
                          ? aReflowState.availableHeight
                          : kidRect.height;

  
  
  nsHTMLReflowState reflowState(aReflowState);
  nsPoint offset = AdjustReflowStateForScrollbars(&reflowState);
  
#ifdef DEBUG
  nsIFrame* f;
  mFixedContainer.FirstChild(this, nsGkAtoms::fixedList, &f);
  NS_ASSERTION(!f || (offset.x == 0 && offset.y == 0),
               "We don't handle correct positioning of fixed frames with "
               "scrollbars in odd positions");
#endif

  
  rv = mFixedContainer.Reflow(this, aPresContext, reflowState,
                              reflowState.ComputedWidth(),
                              reflowState.ComputedHeight(),
                              PR_TRUE, PR_TRUE); 

  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    nsRect damageRect(0, 0, aDesiredSize.width, aDesiredSize.height);
    Invalidate(damageRect, PR_FALSE);
  }

  
  aDesiredSize.mOverflowArea =
    nsRect(nsPoint(0, 0), nsSize(aDesiredSize.width, aDesiredSize.height));

  NS_FRAME_TRACE_REFLOW_OUT("ViewportFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv; 
}

nsIAtom*
ViewportFrame::GetType() const
{
  return nsGkAtoms::viewportFrame;
}

 PRBool
ViewportFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

void
ViewportFrame::InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRBool aImmediate)
{
  nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(this);
  if (parent) {
    nsPoint pt = GetOffsetTo(parent);
    parent->InvalidateInternal(aDamageRect, aX + pt.x, aY + pt.y, this, aImmediate);
    return;
  }
  InvalidateRoot(aDamageRect, aX, aY, aImmediate);
}

#ifdef DEBUG
NS_IMETHODIMP
ViewportFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Viewport"), aResult);
}
#endif
