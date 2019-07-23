




































#include "nsHTMLButtonControlFrame.h"

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsHTMLParts.h"
#include "nsIFormControl.h"

#include "nsIRenderingContext.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsISupports.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsStyleConsts.h"
#include "nsIComponentManager.h"
#include "nsIDocument.h"
#include "nsButtonFrameRenderer.h"
#include "nsFormControlFrame.h"
#include "nsFrameManager.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsStyleSet.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsDisplayList.h"

nsIFrame*
NS_NewHTMLButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsHTMLButtonControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLButtonControlFrame)

nsHTMLButtonControlFrame::nsHTMLButtonControlFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext)
{
}

nsHTMLButtonControlFrame::~nsHTMLButtonControlFrame()
{
}

void
nsHTMLButtonControlFrame::Destroy()
{
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);
  nsHTMLContainerFrame::Destroy();
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::Init(
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_SUCCEEDED(rv)) {
    mRenderer.SetFrame(this, PresContext());
  }
  return rv;
}

NS_QUERYFRAME_HEAD(nsHTMLButtonControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsHTMLButtonControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    nsIContent* content = GetContent();
    nsCOMPtr<nsIDOMHTMLButtonElement> buttonElement(do_QueryInterface(content));
    if (buttonElement) 
      return accService->CreateHTML4ButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
    nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(content));
    if (inputElement) 
      return accService->CreateHTMLButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

nsIAtom*
nsHTMLButtonControlFrame::GetType() const
{
  return nsGkAtoms::HTMLButtonControlFrame;
}

void 
nsHTMLButtonControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
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


NS_IMETHODIMP
nsHTMLButtonControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists)
{
  nsDisplayList onTop;
  if (IsVisibleForPainting(aBuilder)) {
    nsresult rv = mRenderer.DisplayButton(aBuilder, aLists.BorderBackground(), &onTop);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  nsDisplayListCollection set;
  
  if (!aBuilder->IsForEventDelivery()) {
    nsresult rv =
      BuildDisplayListForChild(aBuilder, mFrames.FirstChild(), aDirtyRect, set,
                               DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT);
    NS_ENSURE_SUCCESS(rv, rv);
    
  }
  
  
  set.Content()->AppendToTop(&onTop);

  
  if (IsInput()) {
    nsMargin border = GetStyleBorder()->GetActualBorder();
    nsRect rect(aBuilder->ToReferenceFrame(this), GetSize());
    rect.Deflate(border);
  
    nsresult rv = OverflowClip(aBuilder, set, aLists, rect);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    set.MoveTo(aLists);
  }
  
  nsresult rv = DisplayOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return DisplaySelectionOverlay(aBuilder, aLists);
}

nscoord
nsHTMLButtonControlFrame::GetMinWidth(nsIRenderingContext* aRenderingContext)
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
nsHTMLButtonControlFrame::GetPrefWidth(nsIRenderingContext* aRenderingContext)
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
    nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_TRUE);
  }

  
  nsIFrame* firstKid = mFrames.FirstChild();

  
  
  
  
  
  nsMargin focusPadding = mRenderer.GetAddedButtonBorderAndPadding();
  
  
  ReflowButtonContents(aPresContext, aDesiredSize, aReflowState, firstKid,
                       focusPadding, aStatus);

  aDesiredSize.width = aReflowState.ComputedWidth();

  
  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) 
    aDesiredSize.height = aReflowState.ComputedHeight();
  else
    aDesiredSize.height += focusPadding.TopBottom();
  
  aDesiredSize.width += aReflowState.mComputedBorderPadding.LeftRight();
  aDesiredSize.height += aReflowState.mComputedBorderPadding.TopBottom();

  
  

  
  
  aDesiredSize.height = NS_CSS_MINMAX(aDesiredSize.height,
                                      aReflowState.mComputedMinHeight,
                                      aReflowState.mComputedMaxHeight);

  aDesiredSize.ascent +=
    aReflowState.mComputedBorderPadding.top + focusPadding.top;

  aDesiredSize.mOverflowArea =
    nsRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  ConsiderChildOverflow(aDesiredSize.mOverflowArea, firstKid);
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

void
nsHTMLButtonControlFrame::ReflowButtonContents(nsPresContext* aPresContext,
                                               nsHTMLReflowMetrics& aDesiredSize,
                                               const nsHTMLReflowState& aReflowState,
                                               nsIFrame* aFirstKid,
                                               nsMargin aFocusPadding,
                                               nsReflowStatus& aStatus)
{
  nsSize availSize(aReflowState.ComputedWidth(), NS_INTRINSICSIZE);

  
  
  availSize.width -= aFocusPadding.LeftRight();
  
  
  
  
  
  nscoord xoffset = aFocusPadding.left + aReflowState.mComputedBorderPadding.left;
  nscoord extrawidth = GetMinWidth(aReflowState.rendContext) -
    aReflowState.ComputedWidth();
  if (extrawidth > 0) {
    nscoord extraleft = extrawidth / 2;
    nscoord extraright = extrawidth - extraleft;
    NS_ASSERTION(extraright >=0, "How'd that happen?");
    
    
    extraleft = PR_MIN(extraleft, aReflowState.mComputedPadding.left);
    extraright = PR_MIN(extraright, aReflowState.mComputedPadding.right);
    xoffset -= extraleft;
    availSize.width += extraleft + extraright;
  }
  availSize.width = PR_MAX(availSize.width,0);
  
  nsHTMLReflowState reflowState(aPresContext, aReflowState, aFirstKid,
                                availSize);

  ReflowChild(aFirstKid, aPresContext, aDesiredSize, reflowState,
              xoffset,
              aFocusPadding.top + aReflowState.mComputedBorderPadding.top,
              0, aStatus);
  
  
  
  nscoord minInternalHeight = aReflowState.mComputedMinHeight -
    aReflowState.mComputedBorderPadding.TopBottom();
  minInternalHeight = PR_MAX(minInternalHeight, 0);

  
  nscoord yoff = 0;
  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    yoff = (aReflowState.ComputedHeight() - aDesiredSize.height)/2;
    if (yoff < 0) {
      yoff = 0;
    }
  } else if (aDesiredSize.height < minInternalHeight) {
    yoff = (minInternalHeight - aDesiredSize.height) / 2;
  }

  
  FinishReflowChild(aFirstKid, aPresContext, &reflowState, aDesiredSize,
                    xoffset,
                    yoff + aFocusPadding.top + aReflowState.mComputedBorderPadding.top, 0);

  if (aDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
    aDesiredSize.ascent = aFirstKid->GetBaseline();

  
  
  aDesiredSize.ascent += yoff;
}

 PRBool
nsHTMLButtonControlFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

PRIntn
nsHTMLButtonControlFrame::GetSkipSides() const
{
  return 0;
}

nsresult nsHTMLButtonControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  if (nsGkAtoms::value == aName) {
    return mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::value,
                             aValue, PR_TRUE);
  }
  return NS_OK;
}

nsresult nsHTMLButtonControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  if (nsGkAtoms::value == aName)
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue);

  return NS_OK;
}

nsStyleContext*
nsHTMLButtonControlFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  return mRenderer.GetStyleContext(aIndex);
}

void
nsHTMLButtonControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                    nsStyleContext* aStyleContext)
{
  mRenderer.SetStyleContext(aIndex, aStyleContext);
}

NS_IMETHODIMP 
nsHTMLButtonControlFrame::AppendFrames(nsIAtom*        aListName,
                                       nsFrameList&    aFrameList)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::InsertFrames(nsIAtom*        aListName,
                                       nsIFrame*       aPrevFrame,
                                       nsFrameList&    aFrameList)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::RemoveFrame(nsIAtom*        aListName,
                                      nsIFrame*       aOldFrame)
{
  NS_NOTREACHED("unsupported operation");
  return NS_ERROR_UNEXPECTED;
}
