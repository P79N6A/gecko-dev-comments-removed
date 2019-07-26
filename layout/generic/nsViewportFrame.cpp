









#include "nsViewportFrame.h"
#include "nsGkAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsSubDocumentFrame.h"
#include "nsAbsoluteContainingBlock.h"
#include "GeckoProfiler.h"

using namespace mozilla;

ViewportFrame*
NS_NewViewportFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) ViewportFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(ViewportFrame)
NS_QUERYFRAME_HEAD(ViewportFrame)
  NS_QUERYFRAME_ENTRY(ViewportFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

void
ViewportFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  Super::Init(aContent, aParent, aPrevInFlow);

  nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(this);
  if (parent) {
    nsFrameState state = parent->GetStateBits();

    mState |= state & (NS_FRAME_IN_POPUP);
  }
}

nsresult
ViewportFrame::SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList)
{
  
#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  return nsContainerFrame::SetInitialChildList(aListID, aChildList);
}

void
ViewportFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  PROFILER_LABEL("ViewportFrame", "BuildDisplayList");
  nsIFrame* kid = mFrames.FirstChild();
  if (!kid)
    return;

  
  
  
  BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
}

nsresult
ViewportFrame::AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList ||
               aListID == GetAbsoluteListID(), "unexpected child list");
  NS_ASSERTION(aListID != GetAbsoluteListID() ||
               GetChildList(aListID).IsEmpty(), "Shouldn't have any kids!");
  return nsContainerFrame::AppendFrames(aListID, aFrameList);
}

nsresult
ViewportFrame::InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList ||
               aListID == GetAbsoluteListID(), "unexpected child list");
  NS_ASSERTION(aListID != GetAbsoluteListID() ||
               GetChildList(aListID).IsEmpty(), "Shouldn't have any kids!");
  return nsContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
}

nsresult
ViewportFrame::RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList ||
               aListID == GetAbsoluteListID(), "unexpected child list");
  return nsContainerFrame::RemoveFrame(aListID, aOldFrame);
}

 nscoord
ViewportFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
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
ViewportFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
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
  nsIScrollableFrame* scrollingFrame = do_QueryFrame(kidFrame);

  if (scrollingFrame) {
    nsMargin scrollbars = scrollingFrame->GetActualScrollbarSizes();
    aReflowState->SetComputedWidth(aReflowState->ComputedWidth() -
                                   scrollbars.LeftRight());
    aReflowState->AvailableWidth() -= scrollbars.LeftRight();
    aReflowState->SetComputedHeightWithoutResettingResizeFlags(
      aReflowState->ComputedHeight() - scrollbars.TopBottom());
    return nsPoint(scrollbars.left, scrollbars.top);
  }
  return nsPoint(0, 0);
}

nsRect
ViewportFrame::AdjustReflowStateAsContainingBlock(nsHTMLReflowState* aReflowState) const
{
#ifdef DEBUG
  nsPoint offset =
#endif
    AdjustReflowStateForScrollbars(aReflowState);

  NS_ASSERTION(GetAbsoluteContainingBlock()->GetChildList().IsEmpty() ||
               (offset.x == 0 && offset.y == 0),
               "We don't handle correct positioning of fixed frames with "
               "scrollbars in odd positions");

  
  
  nsRect rect(0, 0, aReflowState->ComputedWidth(), aReflowState->ComputedHeight());
  nsIPresShell* ps = PresContext()->PresShell();
  if (ps->IsScrollPositionClampingScrollPortSizeSet()) {
    rect.SizeTo(ps->GetScrollPositionClampingScrollPortSize());
  }

  
  rect.Deflate(ps->GetContentDocumentFixedPositionMargins());
  return rect;
}

void
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

  
  
  
  SetSize(nsSize(aReflowState.ComputedWidth(), aReflowState.ComputedHeight()));
 
  
  
  
  nscoord kidHeight = 0;

  if (mFrames.NotEmpty()) {
    
    
    if (aReflowState.ShouldReflowAllKids() ||
        aReflowState.mFlags.mVResize ||
        NS_SUBTREE_DIRTY(mFrames.FirstChild())) {
      
      nsIFrame*           kidFrame = mFrames.FirstChild();
      nsHTMLReflowMetrics kidDesiredSize(aReflowState);
      nsSize              availableSpace(aReflowState.AvailableWidth(),
                                         aReflowState.AvailableHeight());
      nsHTMLReflowState   kidReflowState(aPresContext, aReflowState,
                                         kidFrame, availableSpace);

      
      kidReflowState.SetComputedHeight(aReflowState.ComputedHeight());
      ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                  0, 0, 0, aStatus);
      kidHeight = kidDesiredSize.Height();

      FinishReflowChild(kidFrame, aPresContext, kidDesiredSize, nullptr, 0, 0, 0);
    } else {
      kidHeight = mFrames.FirstChild()->GetSize().height;
    }
  }

  NS_ASSERTION(aReflowState.AvailableWidth() != NS_UNCONSTRAINEDSIZE,
               "shouldn't happen anymore");

  
  aDesiredSize.Width() = aReflowState.AvailableWidth();
  
  
  aDesiredSize.Height() = aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE
                          ? aReflowState.ComputedHeight()
                          : kidHeight;
  aDesiredSize.SetOverflowAreasToDesiredBounds();

  if (mFrames.NotEmpty()) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, mFrames.FirstChild());
  }

  if (IsAbsoluteContainer()) {
    
    
    nsHTMLReflowState reflowState(aReflowState);

    if (reflowState.AvailableHeight() == NS_UNCONSTRAINEDSIZE) {
      
      
      reflowState.AvailableHeight() = aDesiredSize.Height();
      
      NS_ASSERTION(reflowState.ComputedPhysicalBorderPadding() == nsMargin(0,0,0,0),
                   "Viewports can't have border/padding");
      reflowState.SetComputedHeight(aDesiredSize.Height());
    }

    nsRect rect = AdjustReflowStateAsContainingBlock(&reflowState);

    
    GetAbsoluteContainingBlock()->Reflow(this, aPresContext, reflowState, aStatus,
                                         rect,
                                         false, true, true, 
                                         &aDesiredSize.mOverflowAreas);
  }

  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    InvalidateFrame();
  }

  
  
  bool overflowChanged = FinishAndStoreOverflow(&aDesiredSize);
  if (overflowChanged) {
    
    
    nsSubDocumentFrame* container = static_cast<nsSubDocumentFrame*>
      (nsLayoutUtils::GetCrossDocParentFrame(this));
    if (container && !container->ShouldClipSubdocument()) {
      container->PresContext()->PresShell()->
        FrameNeedsReflow(container, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    }
  }

  NS_FRAME_TRACE_REFLOW_OUT("ViewportFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
ViewportFrame::GetType() const
{
  return nsGkAtoms::viewportFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
ViewportFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Viewport"), aResult);
}
#endif
