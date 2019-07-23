





































#include "nsCOMPtr.h"
#include "nsTitleBarFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsPIDOMWindow.h"
#include "nsIViewManager.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsDisplayList.h"






nsIFrame*
NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTitleBarFrame(aPresShell, aContext);
} 

nsTitleBarFrame::nsTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
:nsBoxFrame(aPresShell, aContext, PR_FALSE)
{
  mTrackingMouseMove = PR_FALSE;
}



NS_IMETHODIMP
nsTitleBarFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        asPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, asPrevInFlow);

  CreateViewForFrame(PresContext(), this, GetStyleContext(), PR_TRUE);

  return rv;
}

NS_IMETHODIMP
nsTitleBarFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  
  if (aBuilder->IsForEventDelivery()) {
    if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::allowevents,
                               nsGkAtoms::_true, eCaseMatters))
      return NS_OK;
  }
  return nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsTitleBarFrame::HandleEvent(nsPresContext* aPresContext,
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{


  PRBool doDefault = PR_TRUE;

  switch (aEvent->message) {

   case NS_MOUSE_BUTTON_DOWN:  {
       if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
             nsMouseEvent::eLeftButton)
       {

         
         mTrackingMouseMove = PR_TRUE;

         
         CaptureMouseEvents(aPresContext,PR_TRUE);



         
         mLastPoint = aEvent->refPoint;

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = PR_FALSE;
       }
     }
     break;


   case NS_MOUSE_BUTTON_UP: {
       if(mTrackingMouseMove && aEvent->eventStructType == NS_MOUSE_EVENT &&
          NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
            nsMouseEvent::eLeftButton)
       {
         
         mTrackingMouseMove = PR_FALSE;

         
         CaptureMouseEvents(aPresContext,PR_FALSE);

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = PR_FALSE;
       }
     }
     break;

   case NS_MOUSE_MOVE: {
       if(mTrackingMouseMove)
       {
         
         nsPIDOMWindow *window =
           aPresContext->PresShell()->GetDocument()->GetWindow();

         if (window) {
           nsPoint nsMoveBy = aEvent->refPoint - mLastPoint;
           window->MoveBy(nsMoveBy.x,nsMoveBy.y);
         }

         *aEventStatus = nsEventStatus_eConsumeNoDefault;

         doDefault = PR_FALSE;
       }
     }
     break;



    case NS_MOUSE_CLICK:
      if (NS_IS_MOUSE_LEFT_CLICK(aEvent))
      {
        MouseClicked(aPresContext, aEvent);
      }
      break;
  }

  if ( doDefault )
    return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  else
    return NS_OK;
}

NS_IMETHODIMP
nsTitleBarFrame::CaptureMouseEvents(nsPresContext* aPresContext,PRBool aGrabMouseEvents)
{
  
  nsIView* view = GetView();
  PRBool result;

  if (view) {
    nsIViewManager* viewMan = view->GetViewManager();
    if (viewMan) {
      
      if (aGrabMouseEvents) {
        viewMan->GrabMouseEvents(view,result);
        
        
      } else {
        viewMan->GrabMouseEvents(nsnull,result);
        
        
      }
    }
  }

  return NS_OK;

}



void
nsTitleBarFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent)
{
  
  nsEventStatus status = nsEventStatus_eIgnore;

  nsXULCommandEvent event(aEvent ? NS_IS_TRUSTED_EVENT(aEvent) : PR_FALSE,
                          NS_XUL_COMMAND, nsnull);

  nsEventDispatcher::Dispatch(mContent, aPresContext, &event, nsnull, &status);
}
