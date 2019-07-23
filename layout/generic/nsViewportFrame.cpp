









































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

NS_IMPL_FRAMEARENA_HELPERS(ViewportFrame)

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
                                   nsFrameList&    aChildList)
{
  nsresult rv = NS_OK;

  
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  if (nsGkAtoms::fixedList == aListName) {
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
  
  
  
  
  aBuilder->MarkFramesForDisplayList(this, mFixedContainer.GetChildList(),
                                     aDirtyRect);

  nsIFrame* kid = mFrames.FirstChild();
  if (!kid)
    return NS_OK;

  
  
  
  return BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
}

NS_IMETHODIMP
ViewportFrame::AppendFrames(nsIAtom*        aListName,
                            nsFrameList&    aFrameList)
{
  nsresult rv = NS_OK;

  if (nsGkAtoms::fixedList == aListName) {
    rv = mFixedContainer.AppendFrames(this, aListName, aFrameList);
  }
  else {
    NS_ASSERTION(!aListName, "unexpected child list");
    NS_ASSERTION(GetChildList(nsnull).IsEmpty(), "Shouldn't have any kids!");
    rv = nsContainerFrame::AppendFrames(aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
ViewportFrame::InsertFrames(nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  nsresult rv = NS_OK;

  if (nsGkAtoms::fixedList == aListName) {
    rv = mFixedContainer.InsertFrames(this, aListName, aPrevFrame, aFrameList);
  }
  else {
    NS_ASSERTION(!aListName, "unexpected child list");
    NS_ASSERTION(GetChildList(nsnull).IsEmpty(), "Shouldn't have any kids!");
    rv = nsContainerFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
ViewportFrame::RemoveFrame(nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = NS_OK;

  if (nsGkAtoms::fixedList == aListName) {
    mFixedContainer.RemoveFrame(this, aListName, aOldFrame);
    rv = NS_OK;
  }
  else {
    NS_ASSERTION(!aListName, "unexpected child list");
    rv = nsContainerFrame::RemoveFrame(aListName, aOldFrame);
  }

  return rv;
}

nsIAtom*
ViewportFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "illegal index");

  if (0 == aIndex) {
    return nsGkAtoms::fixedList;
  }

  return nsnull;
}

nsFrameList
ViewportFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::fixedList == aListName)
    return mFixedContainer.GetChildList();

  return nsContainerFrame::GetChildList(aListName);
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
  nsIScrollableFrame *scrollingFrame = do_QueryFrame(kidFrame);

  if (scrollingFrame) {
    nsMargin scrollbars = scrollingFrame->GetActualScrollbarSizes();
    aReflowState->SetComputedWidth(aReflowState->ComputedWidth() -
                                   scrollbars.LeftRight());
    aReflowState->availableWidth -= scrollbars.LeftRight();
    aReflowState->SetComputedHeightWithoutResettingResizeFlags(
      aReflowState->ComputedHeight() - scrollbars.TopBottom());
    return nsPoint(scrollbars.left, scrollbars.top);
  }
  return nsPoint(0, 0);
}

NS_IMETHODIMP
ViewportFrame::Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("ViewportFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("ViewportFrame::Reflow");

  
  aStatus = NS_FRAME_COMPLETE;

  
  
  AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  
  
  
  
  nscoord kidHeight = 0;

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

      
      kidReflowState.SetComputedHeight(aReflowState.ComputedHeight());
      rv = ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                       0, 0, 0, aStatus);
      kidHeight = kidDesiredSize.height;

      FinishReflowChild(kidFrame, aPresContext, nsnull, kidDesiredSize, 0, 0, 0);
    } else {
      kidHeight = mFrames.FirstChild()->GetSize().height;
    }
  }

  NS_ASSERTION(aReflowState.availableWidth != NS_UNCONSTRAINEDSIZE,
               "shouldn't happen anymore");

  
  aDesiredSize.width = aReflowState.availableWidth;
  
  
  aDesiredSize.height = aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE
                          ? aReflowState.ComputedHeight()
                          : kidHeight;

  
  
  nsHTMLReflowState reflowState(aReflowState);
  nsPoint offset = AdjustReflowStateForScrollbars(&reflowState);
  
#ifdef DEBUG
  NS_ASSERTION(mFixedContainer.GetChildList().IsEmpty() ||
               (offset.x == 0 && offset.y == 0),
               "We don't handle correct positioning of fixed frames with "
               "scrollbars in odd positions");
#endif

  
  rv = mFixedContainer.Reflow(this, aPresContext, reflowState, aStatus,
                              reflowState.ComputedWidth(),
                              reflowState.ComputedHeight(),
                              PR_FALSE, PR_TRUE, PR_TRUE); 

  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    nsRect damageRect(0, 0, aDesiredSize.width, aDesiredSize.height);
    Invalidate(damageRect);
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
                                  PRUint32 aFlags)
{
  nsRect r = aDamageRect + nsPoint(aX, aY);
  PresContext()->NotifyInvalidation(r, aFlags);

  nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(this);
  if (parent) {
    nsPoint pt = GetOffsetTo(parent);
    parent->InvalidateInternal(r, pt.x, pt.y, this,
                               aFlags | INVALIDATE_CROSS_DOC);
    return;
  }
  InvalidateRoot(r, aFlags);
}

#ifdef DEBUG
NS_IMETHODIMP
ViewportFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Viewport"), aResult);
}
#endif
