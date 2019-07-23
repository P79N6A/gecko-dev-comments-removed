




































#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIEventStateManager.h"
#include "nsILookAndFeel.h"



const PRInt32 kSizeNotSet = -1;

nsFormControlFrame::nsFormControlFrame(nsStyleContext* aContext) :
  nsLeafFrame(aContext)
{
}

nsFormControlFrame::~nsFormControlFrame()
{
}

void
nsFormControlFrame::Destroy()
{
  
  nsFormControlFrame::RegUnRegAccessKey(NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);
  nsLeafFrame::Destroy();
}


NS_IMETHODIMP
nsFormControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIFormControlFrame))) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  return nsLeafFrame::QueryInterface(aIID, aInstancePtr);
}

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

NS_METHOD
nsFormControlFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFormControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (mState & NS_FRAME_FIRST_REFLOW) {
    RegUnRegAccessKey(NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
  }

  return nsLeafFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                             aStatus);
}

nsresult
nsFormControlFrame::RegUnRegAccessKey(nsIFrame * aFrame, PRBool aDoReg)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  
  nsPresContext* presContext = aFrame->GetPresContext();
  
  NS_ASSERTION(presContext, "aPresContext is NULL in RegUnRegAccessKey!");

  nsAutoString accessKey;

  nsIContent* content = aFrame->GetContent();
  content->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);
  if (!accessKey.IsEmpty()) {
    nsIEventStateManager *stateManager = presContext->EventStateManager();
    if (aDoReg) {
      return stateManager->RegisterAccessKey(content, (PRUint32)accessKey.First());
    } else {
      return stateManager->UnregisterAccessKey(content, (PRUint32)accessKey.First());
    }
  }
  return NS_ERROR_FAILURE;
}

void 
nsFormControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}

NS_METHOD
nsFormControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                          nsGUIEvent* aEvent,
                                          nsEventStatus* aEventStatus)
{
  
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
      uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  return NS_OK;
}

void
nsFormControlFrame::GetCurrentCheckState(PRBool *aState)
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

nsresult
nsFormControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  aValue.Truncate();
  return NS_OK;
}

nsresult 
nsFormControlFrame::GetScreenHeight(nsPresContext* aPresContext,
                                    nscoord& aHeight)
{
  nsRect screen;

  nsIDeviceContext *context = aPresContext->DeviceContext();
  PRBool dropdownCanOverlapOSBar = PR_FALSE;
  nsILookAndFeel *lookAndFeel = aPresContext->LookAndFeel();
  lookAndFeel->GetMetric(nsILookAndFeel::eMetric_MenusCanOverlapOSBar,
                         dropdownCanOverlapOSBar);
  if ( dropdownCanOverlapOSBar )
    context->GetRect ( screen );
  else
    context->GetClientRect(screen);

  aHeight = aPresContext->AppUnitsToDevPixels(screen.height);
  return NS_OK;
}
