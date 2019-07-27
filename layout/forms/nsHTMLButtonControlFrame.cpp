




#include "nsHTMLButtonControlFrame.h"

#include "nsContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsButtonFrameRenderer.h"
#include "nsCSSAnonBoxes.h"
#include "nsFormControlFrame.h"
#include "nsNameSpaceManager.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include <algorithm>

using namespace mozilla;

nsContainerFrame*
NS_NewHTMLButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsHTMLButtonControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLButtonControlFrame)

nsHTMLButtonControlFrame::nsHTMLButtonControlFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
{
}

nsHTMLButtonControlFrame::~nsHTMLButtonControlFrame()
{
}

void
nsHTMLButtonControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

void
nsHTMLButtonControlFrame::Init(nsIContent*       aContent,
                               nsContainerFrame* aParent,
                               nsIFrame*         aPrevInFlow)
{
  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);
  mRenderer.SetFrame(this, PresContext());
}

NS_QUERYFRAME_HEAD(nsHTMLButtonControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsHTMLButtonControlFrame::AccessibleType()
{
  return a11y::eHTMLButtonType;
}
#endif

nsIAtom*
nsHTMLButtonControlFrame::GetType() const
{
  return nsGkAtoms::HTMLButtonControlFrame;
}

void 
nsHTMLButtonControlFrame::SetFocus(bool aOn, bool aRepaint)
{
}

nsresult
nsHTMLButtonControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                      WidgetGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  
  if (mRenderer.isDisabled()) {
    return NS_OK;
  }

  
  
  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}


void
nsHTMLButtonControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists)
{
  
  Maybe<DisplayListClipState::AutoSaveRestore> eventClipState;
  const bool isForEventDelivery = aBuilder->IsForEventDelivery();
  if (isForEventDelivery) {
    eventClipState.emplace(aBuilder);
    nsRect rect(aBuilder->ToReferenceFrame(this), GetSize());
    nscoord radii[8];
    bool hasRadii = GetBorderRadii(radii);
    eventClipState->ClipContainingBlockDescendants(rect, hasRadii ? radii : nullptr);
  }

  nsDisplayList onTop;
  if (IsVisibleForPainting(aBuilder)) {
    mRenderer.DisplayButton(aBuilder, aLists.BorderBackground(), &onTop);
  }

  nsDisplayListCollection set;

  
  if (!isForEventDelivery) {
    DisplayListClipState::AutoSaveRestore clipState(aBuilder);

    if (IsInput() || StyleDisplay()->mOverflowX != NS_STYLE_OVERFLOW_VISIBLE) {
      nsMargin border = StyleBorder()->GetComputedBorder();
      nsRect rect(aBuilder->ToReferenceFrame(this), GetSize());
      rect.Deflate(border);
      nscoord radii[8];
      bool hasRadii = GetPaddingBoxBorderRadii(radii);
      clipState.ClipContainingBlockDescendants(rect, hasRadii ? radii : nullptr);
    }

    BuildDisplayListForChild(aBuilder, mFrames.FirstChild(), aDirtyRect, set,
                             DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT);
    
  }
  
  
  set.Content()->AppendToTop(&onTop);
  set.MoveTo(aLists);
  
  DisplayOutline(aBuilder, aLists);

  
  DisplaySelectionOverlay(aBuilder, aLists.Content());
}

nscoord
nsHTMLButtonControlFrame::GetMinISize(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame* kid = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                kid,
                                                nsLayoutUtils::MIN_ISIZE);

  result += GetWritingMode().IsVertical()
    ? mRenderer.GetAddedButtonBorderAndPadding().TopBottom()
    : mRenderer.GetAddedButtonBorderAndPadding().LeftRight();

  return result;
}

nscoord
nsHTMLButtonControlFrame::GetPrefISize(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  
  nsIFrame* kid = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                kid,
                                                nsLayoutUtils::PREF_ISIZE);

  result += GetWritingMode().IsVertical()
    ? mRenderer.GetAddedButtonBorderAndPadding().TopBottom()
    : mRenderer.GetAddedButtonBorderAndPadding().LeftRight();

  return result;
}

void
nsHTMLButtonControlFrame::Reflow(nsPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsHTMLButtonControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_PRECONDITION(aReflowState.ComputedISize() != NS_INTRINSICSIZE,
                  "Should have real computed inline-size by now");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), true);
  }

  
  nsIFrame* firstKid = mFrames.FirstChild();

  MOZ_ASSERT(firstKid, "Button should have a child frame for its contents");
  MOZ_ASSERT(!firstKid->GetNextSibling(),
             "Button should have exactly one child frame");
  MOZ_ASSERT(firstKid->StyleContext()->GetPseudo() ==
             nsCSSAnonBoxes::buttonContent,
             "Button's child frame has unexpected pseudo type!");

  
  
  
  

  
  
  ReflowButtonContents(aPresContext, aDesiredSize,
                       aReflowState, firstKid);

  ConsiderChildOverflow(aDesiredSize.mOverflowAreas, firstKid);

  aStatus = NS_FRAME_COMPLETE;
  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize,
                                 aReflowState, aStatus);

  
  
  aStatus = NS_FRAME_COMPLETE;
  MOZ_ASSERT(!GetNextInFlow());

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}






static nsHTMLReflowState
CloneReflowStateWithReducedContentBox(
  const nsHTMLReflowState& aButtonReflowState,
  const nsMargin& aFocusPadding)
{
  nscoord adjustedWidth =
    aButtonReflowState.ComputedWidth() - aFocusPadding.LeftRight();
  adjustedWidth = std::max(0, adjustedWidth);

  
  nscoord adjustedHeight = aButtonReflowState.ComputedHeight();
  if (adjustedHeight != NS_INTRINSICSIZE) {
    adjustedHeight -= aFocusPadding.TopBottom();
    adjustedHeight = std::max(0, adjustedHeight);
  }

  nsHTMLReflowState clone(aButtonReflowState);
  clone.SetComputedWidth(adjustedWidth);
  clone.SetComputedHeight(adjustedHeight);

  return clone;
}

void
nsHTMLButtonControlFrame::ReflowButtonContents(nsPresContext* aPresContext,
                                               nsHTMLReflowMetrics& aButtonDesiredSize,
                                               const nsHTMLReflowState& aButtonReflowState,
                                               nsIFrame* aFirstKid)
{
  WritingMode wm = GetWritingMode();
  LogicalSize availSize = aButtonReflowState.ComputedSize(wm);
  availSize.BSize(wm) = NS_INTRINSICSIZE;

  
  
  LogicalMargin focusPadding =
    LogicalMargin(wm, mRenderer.GetAddedButtonBorderAndPadding());

  
  
  
  
  
  
  nscoord IOverflow = GetMinISize(aButtonReflowState.rendContext) -
                      aButtonReflowState.ComputedISize();
  nscoord IFocusPadding = focusPadding.IStartEnd(wm);
  nscoord focusPaddingReduction = std::min(IFocusPadding,
                                           std::max(IOverflow, 0));
  if (focusPaddingReduction > 0) {
    nscoord startReduction = focusPadding.IStart(wm);
    if (focusPaddingReduction != IFocusPadding) {
      startReduction = NSToCoordRound(startReduction *
                                      (float(focusPaddingReduction) /
                                       float(IFocusPadding)));
    }
    focusPadding.IStart(wm) -= startReduction;
    focusPadding.IEnd(wm) -= focusPaddingReduction - startReduction;
  }

  
  const LogicalMargin& clbp = aButtonReflowState.ComputedLogicalBorderPadding();

  
  
  availSize.ISize(wm) -= focusPadding.IStartEnd(wm);

  LogicalPoint childPos(wm);
  childPos.I(wm) = focusPadding.IStart(wm) + clbp.IStart(wm);
  availSize.ISize(wm) = std::max(availSize.ISize(wm), 0);

  
  
  nsHTMLReflowState adjustedButtonReflowState =
    CloneReflowStateWithReducedContentBox(aButtonReflowState,
                                          focusPadding.GetPhysicalMargin(wm));

  nsHTMLReflowState contentsReflowState(aPresContext,
                                        adjustedButtonReflowState,
                                        aFirstKid, availSize);

  nsReflowStatus contentsReflowStatus;
  nsHTMLReflowMetrics contentsDesiredSize(aButtonReflowState);
  childPos.B(wm) = 0; 
                      

  
  
  nsSize dummyContainerSize;
  ReflowChild(aFirstKid, aPresContext,
              contentsDesiredSize, contentsReflowState,
              wm, childPos, dummyContainerSize, 0, contentsReflowStatus);
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(contentsReflowStatus),
             "We gave button-contents frame unconstrained available height, "
             "so it should be complete");

  
  LogicalSize buttonContentBox(wm);
  if (aButtonReflowState.ComputedBSize() != NS_INTRINSICSIZE) {
    
    buttonContentBox.BSize(wm) = aButtonReflowState.ComputedBSize();
  } else {
    
    
    buttonContentBox.BSize(wm) =
      contentsDesiredSize.BSize(wm) + focusPadding.BStartEnd(wm);

    
    
    
    
    
    buttonContentBox.BSize(wm) =
      NS_CSS_MINMAX(buttonContentBox.BSize(wm),
                    aButtonReflowState.ComputedMinBSize(),
                    aButtonReflowState.ComputedMaxBSize());
  }
  if (aButtonReflowState.ComputedISize() != NS_INTRINSICSIZE) {
    buttonContentBox.ISize(wm) = aButtonReflowState.ComputedISize();
  } else {
    buttonContentBox.ISize(wm) =
      contentsDesiredSize.ISize(wm) + focusPadding.IStartEnd(wm);
    buttonContentBox.ISize(wm) =
      NS_CSS_MINMAX(buttonContentBox.ISize(wm),
                    aButtonReflowState.ComputedMinISize(),
                    aButtonReflowState.ComputedMaxISize());
  }

  
  
  nscoord extraSpace =
    buttonContentBox.BSize(wm) - focusPadding.BStartEnd(wm) -
    contentsDesiredSize.BSize(wm);

  childPos.B(wm) = std::max(0, extraSpace / 2);

  
  
  childPos.B(wm) += focusPadding.BStart(wm) + clbp.BStart(wm);

  nsSize containerSize =
    (buttonContentBox + clbp.Size(wm)).GetPhysicalSize(wm);

  
  FinishReflowChild(aFirstKid, aPresContext,
                    contentsDesiredSize, &contentsReflowState,
                    wm, childPos, containerSize, 0);

  
  if (contentsDesiredSize.BlockStartAscent() ==
      nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    WritingMode wm = aButtonReflowState.GetWritingMode();
    contentsDesiredSize.SetBlockStartAscent(aFirstKid->GetLogicalBaseline(wm));
  }

  
  
  
  aButtonDesiredSize.SetSize(wm,
    LogicalSize(wm, aButtonReflowState.ComputedISize() + clbp.IStartEnd(wm),
                    buttonContentBox.BSize(wm) + clbp.BStartEnd(wm)));

  
  
  
  
  if (aButtonDesiredSize.GetWritingMode().IsOrthogonalTo(wm)) {
    aButtonDesiredSize.SetBlockStartAscent(contentsDesiredSize.ISize(wm));
  } else {
    aButtonDesiredSize.SetBlockStartAscent(contentsDesiredSize.BlockStartAscent() +
                                           childPos.B(wm));
  }

  aButtonDesiredSize.SetOverflowAreasToDesiredBounds();
}

nsresult nsHTMLButtonControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  if (nsGkAtoms::value == aName) {
    return mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::value,
                             aValue, true);
  }
  return NS_OK;
}

nsStyleContext*
nsHTMLButtonControlFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  return mRenderer.GetStyleContext(aIndex);
}

void
nsHTMLButtonControlFrame::SetAdditionalStyleContext(int32_t aIndex, 
                                                    nsStyleContext* aStyleContext)
{
  mRenderer.SetStyleContext(aIndex, aStyleContext);
}

#ifdef DEBUG
void
nsHTMLButtonControlFrame::AppendFrames(ChildListID     aListID,
                                       nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsHTMLButtonControlFrame::InsertFrames(ChildListID     aListID,
                                       nsIFrame*       aPrevFrame,
                                       nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsHTMLButtonControlFrame::RemoveFrame(ChildListID     aListID,
                                      nsIFrame*       aOldFrame)
{
  MOZ_CRASH("unsupported operation");
}
#endif
