









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
ViewportFrame::Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow)
{
  Super::Init(aContent, aParent, aPrevInFlow);

  nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(this);
  if (parent) {
    nsFrameState state = parent->GetStateBits();

    mState |= state & (NS_FRAME_IN_POPUP);
  }
}

void
ViewportFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  PROFILER_LABEL("ViewportFrame", "BuildDisplayList",
    js::ProfileEntry::Category::GRAPHICS);

  nsIFrame* kid = mFrames.FirstChild();
  if (!kid)
    return;

  
  
  
  BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
}

#ifdef DEBUG
void
ViewportFrame::SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList)
{
  nsFrame::VerifyDirtyBitSet(aChildList);
  nsContainerFrame::SetInitialChildList(aListID, aChildList);
}

void
ViewportFrame::AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(GetChildList(aListID).IsEmpty(), "Shouldn't have any kids!");
  nsContainerFrame::AppendFrames(aListID, aFrameList);
}

void
ViewportFrame::InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(GetChildList(aListID).IsEmpty(), "Shouldn't have any kids!");
  nsContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
}

void
ViewportFrame::RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  nsContainerFrame::RemoveFrame(aListID, aOldFrame);
}
#endif

 nscoord
ViewportFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetMinISize(aRenderingContext);

  return result;
}

 nscoord
ViewportFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetPrefISize(aRenderingContext);

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

  
  
  
  nscoord kidBSize = 0;
  WritingMode wm = aReflowState.GetWritingMode();

  if (mFrames.NotEmpty()) {
    
    
    if (aReflowState.ShouldReflowAllKids() ||
        aReflowState.mFlags.mVResize ||
        NS_SUBTREE_DIRTY(mFrames.FirstChild())) {
      
      nsIFrame*           kidFrame = mFrames.FirstChild();
      nsHTMLReflowMetrics kidDesiredSize(aReflowState);
      WritingMode         wm = kidFrame->GetWritingMode();
      LogicalSize         availableSpace = aReflowState.AvailableSize(wm);
      nsHTMLReflowState   kidReflowState(aPresContext, aReflowState,
                                         kidFrame, availableSpace);

      
      kidReflowState.SetComputedHeight(aReflowState.ComputedHeight());
      ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                  0, 0, 0, aStatus);
      kidBSize = kidDesiredSize.BSize(wm);

      FinishReflowChild(kidFrame, aPresContext, kidDesiredSize, nullptr, 0, 0, 0);
    } else {
      kidBSize = LogicalSize(wm, mFrames.FirstChild()->GetSize()).BSize(wm);
    }
  }

  NS_ASSERTION(aReflowState.AvailableWidth() != NS_UNCONSTRAINEDSIZE,
               "shouldn't happen anymore");

  
  LogicalSize maxSize(wm, aReflowState.AvailableISize(),
                      
                      
                      aReflowState.ComputedBSize() != NS_UNCONSTRAINEDSIZE
                        ? aReflowState.ComputedBSize()
                        : kidBSize);
  aDesiredSize.SetSize(wm, maxSize);
  aDesiredSize.SetOverflowAreasToDesiredBounds();

  if (IsAbsoluteContainer()) {
    
    
    nsHTMLReflowState reflowState(aReflowState);

    if (reflowState.AvailableBSize() == NS_UNCONSTRAINEDSIZE) {
      
      
      reflowState.AvailableBSize() = maxSize.BSize(wm);
      
      NS_ASSERTION(reflowState.ComputedPhysicalBorderPadding() == nsMargin(0,0,0,0),
                   "Viewports can't have border/padding");
      reflowState.SetComputedBSize(maxSize.BSize(wm));
    }

    nsRect rect = AdjustReflowStateAsContainingBlock(&reflowState);

    
    GetAbsoluteContainingBlock()->Reflow(this, aPresContext, reflowState, aStatus,
                                         rect,
                                         false, true, true, 
                                         &aDesiredSize.mOverflowAreas);

    nsIScrollableFrame* rootScrollFrame =
                    aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
    if (rootScrollFrame && !rootScrollFrame->IsIgnoringViewportClipping()) {
      aDesiredSize.SetOverflowAreasToDesiredBounds();
    }
  }

  if (mFrames.NotEmpty()) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, mFrames.FirstChild());
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

bool
ViewportFrame::UpdateOverflow()
{
  nsIScrollableFrame* rootScrollFrame =
    PresContext()->PresShell()->GetRootScrollFrameAsScrollable();
  if (rootScrollFrame && !rootScrollFrame->IsIgnoringViewportClipping()) {
    return false;
  }

  return nsFrame::UpdateOverflow();
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
