




#include "nsHTMLButtonControlFrame.h"

#include "nsContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsButtonFrameRenderer.h"
#include "nsFormControlFrame.h"
#include "nsINameSpaceManager.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include <algorithm>

using namespace mozilla;

nsIFrame*
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
nsHTMLButtonControlFrame::Init(
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIFrame*        aPrevInFlow)
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

NS_IMETHODIMP
nsHTMLButtonControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
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
  nsDisplayList onTop;
  if (IsVisibleForPainting(aBuilder)) {
    mRenderer.DisplayButton(aBuilder, aLists.BorderBackground(), &onTop);
  }

  nsDisplayListCollection set;

  
  if (!aBuilder->IsForEventDelivery()) {
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
nsHTMLButtonControlFrame::GetMinWidth(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame* kid = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                kid,
                                                nsLayoutUtils::MIN_WIDTH);

  result += mRenderer.GetAddedButtonBorderAndPadding().LeftRight();

  return result;
}

nscoord
nsHTMLButtonControlFrame::GetPrefWidth(nsRenderingContext* aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  
  nsIFrame* kid = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                kid,
                                                nsLayoutUtils::PREF_WIDTH);
  result += mRenderer.GetAddedButtonBorderAndPadding().LeftRight();
  return result;
}

NS_IMETHODIMP 
nsHTMLButtonControlFrame::Reflow(nsPresContext* aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsHTMLButtonControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_PRECONDITION(aReflowState.ComputedWidth() != NS_INTRINSICSIZE,
                  "Should have real computed width by now");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), true);
  }

  
  nsIFrame* firstKid = mFrames.FirstChild();

  
  
  
  
  
  nsMargin focusPadding = mRenderer.GetAddedButtonBorderAndPadding();
  
  
  ReflowButtonContents(aPresContext, aDesiredSize, aReflowState, firstKid,
                       focusPadding);

  aDesiredSize.width = aReflowState.ComputedWidth();

  aDesiredSize.width += aReflowState.mComputedBorderPadding.LeftRight();
  aDesiredSize.height += aReflowState.mComputedBorderPadding.TopBottom();

  aDesiredSize.ascent +=
    aReflowState.mComputedBorderPadding.top + focusPadding.top;

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aDesiredSize.mOverflowAreas, firstKid);

  aStatus = NS_FRAME_COMPLETE;
  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize,
                                 aReflowState, aStatus);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

void
nsHTMLButtonControlFrame::ReflowButtonContents(nsPresContext* aPresContext,
                                               nsHTMLReflowMetrics& aDesiredSize,
                                               const nsHTMLReflowState& aButtonReflowState,
                                               nsIFrame* aFirstKid,
                                               nsMargin aFocusPadding)
{
  nsSize availSize(aButtonReflowState.ComputedWidth(), NS_INTRINSICSIZE);

  
  
  availSize.width -= aFocusPadding.LeftRight();
  
  
  
  
  
  nscoord xoffset = aFocusPadding.left +
    aButtonReflowState.mComputedBorderPadding.left;
  nscoord extrawidth = GetMinWidth(aButtonReflowState.rendContext) -
    aButtonReflowState.ComputedWidth();
  if (extrawidth > 0) {
    nscoord extraleft = extrawidth / 2;
    nscoord extraright = extrawidth - extraleft;
    NS_ASSERTION(extraright >=0, "How'd that happen?");
    
    
    extraleft = std::min(extraleft, aButtonReflowState.mComputedPadding.left);
    extraright = std::min(extraright, aButtonReflowState.mComputedPadding.right);
    xoffset -= extraleft;
    availSize.width += extraleft + extraright;
  }
  availSize.width = std::max(availSize.width,0);
  
  nsHTMLReflowState contentsReflowState(aPresContext, aButtonReflowState,
                                        aFirstKid, availSize);

  nsReflowStatus contentsReflowStatus;
  ReflowChild(aFirstKid, aPresContext, aDesiredSize, contentsReflowState,
              xoffset,
              aFocusPadding.top + aButtonReflowState.mComputedBorderPadding.top,
              0, contentsReflowStatus);
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(contentsReflowStatus),
             "We gave button-contents frame unconstrained available height, "
             "so it should be complete");

  
  nscoord actualDesiredHeight = 0;
  if (aButtonReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    actualDesiredHeight = aButtonReflowState.ComputedHeight();
  } else {
    actualDesiredHeight = aDesiredSize.height + aFocusPadding.TopBottom();

    
    
    
    
    
    actualDesiredHeight = NS_CSS_MINMAX(actualDesiredHeight,
                                        aButtonReflowState.mComputedMinHeight,
                                        aButtonReflowState.mComputedMaxHeight);
  }

  
  nscoord yoff = (actualDesiredHeight - aFocusPadding.TopBottom() - aDesiredSize.height) / 2;
  if (yoff < 0) {
    yoff = 0;
  }

  
  FinishReflowChild(aFirstKid, aPresContext, &contentsReflowState, aDesiredSize,
                    xoffset,
                    yoff + aFocusPadding.top +
                      aButtonReflowState.mComputedBorderPadding.top,
                    0);

  if (aDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
    aDesiredSize.ascent = aFirstKid->GetBaseline();

  
  
  aDesiredSize.ascent += yoff;
  aDesiredSize.height = actualDesiredHeight;
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

NS_IMETHODIMP 
nsHTMLButtonControlFrame::AppendFrames(ChildListID     aListID,
                                       nsFrameList&    aFrameList)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::InsertFrames(ChildListID     aListID,
                                       nsIFrame*       aPrevFrame,
                                       nsFrameList&    aFrameList)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::RemoveFrame(ChildListID     aListID,
                                      nsIFrame*       aOldFrame)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}
