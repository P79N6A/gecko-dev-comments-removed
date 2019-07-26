




#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsIDOMHTMLInputElement.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/LookAndFeel.h"
#include "nsDeviceContext.h"

using namespace mozilla;



nsFormControlFrame::nsFormControlFrame(nsStyleContext* aContext) :
  nsLeafFrame(aContext)
{
}

nsFormControlFrame::~nsFormControlFrame()
{
}

nsIAtom*
nsFormControlFrame::GetType() const
{
  return nsGkAtoms::formControlFrame; 
}

void
nsFormControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsLeafFrame::DestroyFrom(aDestructRoot);
}

NS_QUERYFRAME_HEAD(nsFormControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsLeafFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsFormControlFrame)

nscoord
nsFormControlFrame::GetIntrinsicWidth()
{
  
  
  
  return nsPresContext::CSSPixelsToAppUnits(13 - 2 * 2);
}

nscoord
nsFormControlFrame::GetIntrinsicHeight()
{
  
  
  
  return nsPresContext::CSSPixelsToAppUnits(13 - 2 * 2);
}

nscoord
nsFormControlFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  NS_ASSERTION(!NS_SUBTREE_DIRTY(this),
               "frame must not be dirty");
  
  
  
  return BSize(aWritingMode) -
         GetLogicalUsedBorderAndPadding(aWritingMode).BEnd(aWritingMode);
}

void
nsFormControlFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFormControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (mState & NS_FRAME_FIRST_REFLOW) {
    RegUnRegAccessKey(static_cast<nsIFrame*>(this), true);
  }

  nsLeafFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  if (nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
    float inflation = nsLayoutUtils::FontSizeInflationFor(this);
    aDesiredSize.Width() *= inflation;
    aDesiredSize.Height() *= inflation;
    aDesiredSize.UnionOverflowAreasWithDesiredBounds();
    FinishAndStoreOverflow(&aDesiredSize);
  }
}

nsresult
nsFormControlFrame::RegUnRegAccessKey(nsIFrame * aFrame, bool aDoReg)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  
  nsPresContext* presContext = aFrame->PresContext();
  
  NS_ASSERTION(presContext, "aPresContext is NULL in RegUnRegAccessKey!");

  nsAutoString accessKey;

  nsIContent* content = aFrame->GetContent();
  content->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);
  if (!accessKey.IsEmpty()) {
    EventStateManager* stateManager = presContext->EventStateManager();
    if (aDoReg) {
      stateManager->RegisterAccessKey(content, (uint32_t)accessKey.First());
    } else {
      stateManager->UnregisterAccessKey(content, (uint32_t)accessKey.First());
    }
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

void 
nsFormControlFrame::SetFocus(bool aOn, bool aRepaint)
{
}

nsresult
nsFormControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus)
{
  
  const nsStyleUserInterface* uiStyle = StyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
      uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  return NS_OK;
}

void
nsFormControlFrame::GetCurrentCheckState(bool *aState)
{
  nsCOMPtr<nsIDOMHTMLInputElement> inputElement = do_QueryInterface(mContent);
  if (inputElement) {
    inputElement->GetChecked(aState);
  }
}

nsresult
nsFormControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  return NS_OK;
}


nsRect
nsFormControlFrame::GetUsableScreenRect(nsPresContext* aPresContext)
{
  nsRect screen;

  nsDeviceContext *context = aPresContext->DeviceContext();
  int32_t dropdownCanOverlapOSBar =
    LookAndFeel::GetInt(LookAndFeel::eIntID_MenusCanOverlapOSBar, 0);
  if ( dropdownCanOverlapOSBar )
    context->GetRect(screen);
  else
    context->GetClientRect(screen);

  return screen;
}
