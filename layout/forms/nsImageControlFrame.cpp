



































#include "nsCOMPtr.h"
#include "nsImageFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIFormControl.h"
#include "nsHTMLParts.h"
#include "nsIRenderingContext.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsISupports.h"
#include "nsGkAtoms.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleConsts.h"
#include "nsFormControlFrame.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsContainerFrame.h"
#include "nsLayoutUtils.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif

void
IntPointDtorFunc(void *aObject, nsIAtom *aPropertyName,
                 void *aPropertyValue, void *aData)
{
  nsIntPoint *propertyValue = static_cast<nsIntPoint*>(aPropertyValue);
  delete propertyValue;
}


#define nsImageControlFrameSuper nsImageFrame
class nsImageControlFrame : public nsImageControlFrameSuper,
                            public nsIFormControlFrame
{
public:
  nsImageControlFrame(nsStyleContext* aContext);
  ~nsImageControlFrame();

  virtual void Destroy();
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  virtual nsIAtom* GetType() const;

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("ImageControl"), aResult);
  }
#endif

  NS_IMETHOD GetCursor(const nsPoint&    aPoint,
                       nsIFrame::Cursor& aCursor);
  
  virtual void SetFocus(PRBool aOn, PRBool aRepaint);
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
};


nsImageControlFrame::nsImageControlFrame(nsStyleContext* aContext):
  nsImageControlFrameSuper(aContext)
{
}

nsImageControlFrame::~nsImageControlFrame()
{
}

void
nsImageControlFrame::Destroy()
{
  if (!GetPrevInFlow()) {
    nsFormControlFrame::RegUnRegAccessKey(this, PR_FALSE);
  }
  nsImageControlFrameSuper::Destroy();
}

nsIFrame*
NS_NewImageControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsImageControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsImageControlFrame)

NS_IMETHODIMP
nsImageControlFrame::Init(nsIContent*      aContent,
                          nsIFrame*        aParent,
                          nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsImageControlFrameSuper::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aPrevInFlow) {
    return NS_OK;
  }
  
  return  mContent->SetProperty(nsGkAtoms::imageClickedPoint,
                                 new nsIntPoint(0, 0),
                                 IntPointDtorFunc);
}

NS_QUERYFRAME_HEAD(nsImageControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsImageControlFrameSuper)

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsImageControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    if (mContent->Tag() == nsGkAtoms::button) {
      return accService->CreateHTML4ButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
    }
    else if (mContent->Tag() == nsGkAtoms::input) {
      return accService->CreateHTMLButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
    }
  }

  return NS_ERROR_FAILURE;
}
#endif

nsIAtom*
nsImageControlFrame::GetType() const
{
  return nsGkAtoms::imageControlFrame; 
}

NS_METHOD
nsImageControlFrame::Reflow(nsPresContext*         aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsImageControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  if (!GetPrevInFlow() && (mState & NS_FRAME_FIRST_REFLOW)) {
    nsFormControlFrame::RegUnRegAccessKey(this, PR_TRUE);
  }
  return nsImageControlFrameSuper::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}

NS_METHOD 
nsImageControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                 nsGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) { 
    return NS_OK;
  }

  *aEventStatus = nsEventStatus_eIgnore;

  if (aEvent->eventStructType == NS_MOUSE_EVENT &&
      aEvent->message == NS_MOUSE_BUTTON_UP &&
      static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) {
    
    
    nsIntPoint* lastClickPoint =
      static_cast<nsIntPoint*>
                 (mContent->GetProperty(nsGkAtoms::imageClickedPoint));
    if (lastClickPoint) {
      
      nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
      TranslateEventCoords(pt, *lastClickPoint);
    }
  }
  return nsImageControlFrameSuper::HandleEvent(aPresContext, aEvent,
                                               aEventStatus);
}

void 
nsImageControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}

NS_IMETHODIMP
nsImageControlFrame::GetCursor(const nsPoint&    aPoint,
                               nsIFrame::Cursor& aCursor)
{
  
  
  FillCursorInformationFromStyle(GetStyleUserInterface(), aCursor);

  if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
    aCursor.mCursor = NS_STYLE_CURSOR_POINTER;
  }

  return NS_OK;
}

nsresult
nsImageControlFrame::SetFormProperty(nsIAtom* aName,
                                     const nsAString& aValue)
{
  return NS_OK;
}

nsresult
nsImageControlFrame::GetFormProperty(nsIAtom* aName,
                                     nsAString& aValue) const
{
  aValue.Truncate();
  return NS_OK;
}
