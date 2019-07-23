







































#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsGUIEvent.h"
#include "nsButtonBoxFrame.h"
#include "nsITimer.h"
#include "nsRepeatService.h"

class nsAutoRepeatBoxFrame : public nsButtonBoxFrame, 
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  friend nsIFrame* NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

  NS_IMETHOD Init(nsIContent*     aContent,
                  nsIFrame*       aParent,
                  nsIFrame*       aPrevInFlow);

  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD HandlePress(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext, 
                           nsGUIEvent*     aEvent,
                           nsEventStatus*  aEventStatus);

  NS_DECL_NSITIMERCALLBACK

protected:
  nsAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
    nsButtonBoxFrame(aPresShell, aContext) {}

  PRPackedBool mTrustedEvent;
  PRPackedBool mIsPressMode;

  void InitRepeatMode();
};

nsIFrame*
NS_NewAutoRepeatBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsAutoRepeatBoxFrame (aPresShell, aContext);
} 

NS_INTERFACE_MAP_BEGIN(nsAutoRepeatBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_INHERITING(nsButtonBoxFrame)

NS_IMPL_ADDREF_INHERITED(nsAutoRepeatBoxFrame, nsButtonBoxFrame)
NS_IMPL_RELEASE_INHERITED(nsAutoRepeatBoxFrame, nsButtonBoxFrame)

NS_IMETHODIMP
nsAutoRepeatBoxFrame::Init(nsIContent* aContent,
                           nsIFrame* aParent,
                           nsIFrame* aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  InitRepeatMode();

  return rv;
}


NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{  
  switch(aEvent->message)
  {
    case NS_MOUSE_ENTER:
    case NS_MOUSE_ENTER_SYNTH:
      if (!mIsPressMode) {
        nsRepeatService::GetInstance()->Start(this);
        mTrustedEvent = NS_IS_TRUSTED_EVENT(aEvent);
      }
      break;

    case NS_MOUSE_EXIT:
    case NS_MOUSE_EXIT_SYNTH:
      
      nsRepeatService::GetInstance()->Stop();
      
      mTrustedEvent = PR_FALSE;
      break;

    case NS_MOUSE_CLICK:
      if (NS_IS_MOUSE_LEFT_CLICK(aEvent)) {
        
         return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
      }
      break;
  }
     
  return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandlePress(nsPresContext* aPresContext, 
                                  nsGUIEvent* aEvent,
                                  nsEventStatus* aEventStatus)
{
  if (mIsPressMode) {
    mTrustedEvent = NS_IS_TRUSTED_EVENT(aEvent);
    DoMouseClick(aEvent, mTrustedEvent);
    nsRepeatService::GetInstance()->Start(this);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsAutoRepeatBoxFrame::HandleRelease(nsPresContext* aPresContext, 
                                    nsGUIEvent* aEvent,
                                    nsEventStatus* aEventStatus)
{
  if (mIsPressMode) {
    nsRepeatService::GetInstance()->Stop();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       PRInt32 aModType)
{
  if (aAttribute == nsGkAtoms::type) {
    nsRepeatService::GetInstance()->Stop();
    InitRepeatMode();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::Notify(nsITimer *timer)
{
  DoMouseClick(nsnull, mTrustedEvent);
  return NS_OK;
}

void
nsAutoRepeatBoxFrame::Destroy()
{
  
  
  nsRepeatService::GetInstance()->Stop();
  nsButtonBoxFrame::Destroy();
}

void
nsAutoRepeatBoxFrame::InitRepeatMode()
{
  
  
  
  nsAutoString repeat;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::repeat, repeat);
  mIsPressMode = !repeat.EqualsLiteral("hover");
}
