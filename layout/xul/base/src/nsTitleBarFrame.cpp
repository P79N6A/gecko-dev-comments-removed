





































#include "nsCOMPtr.h"
#include "nsTitleBarFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsIWidget.h"
#include "nsMenuPopupFrame.h"
#include "nsPresContext.h"
#include "nsIDocShellTreeItem.h"
#include "nsPIDOMWindow.h"
#include "nsIViewManager.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"






nsIFrame*
NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTitleBarFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTitleBarFrame)

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
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  PRBool doDefault = PR_TRUE;

  switch (aEvent->message) {

   case NS_MOUSE_BUTTON_DOWN:  {
       if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           static_cast<nsMouseEvent*>(aEvent)->button ==
             nsMouseEvent::eLeftButton)
       {
         
         nsCOMPtr<nsISupports> cont = aPresContext->GetContainer();
         nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
         if (dsti) {
           PRInt32 type = -1;
           if (NS_SUCCEEDED(dsti->GetItemType(&type)) &&
               type == nsIDocShellTreeItem::typeChrome) {
             
             mTrackingMouseMove = PR_TRUE;

             
             CaptureMouseEvents(aPresContext,PR_TRUE);

             
             mLastPoint = aEvent->refPoint;
           }
         }

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = PR_FALSE;
       }
     }
     break;


   case NS_MOUSE_BUTTON_UP: {
       if(mTrackingMouseMove && aEvent->eventStructType == NS_MOUSE_EVENT &&
          static_cast<nsMouseEvent*>(aEvent)->button ==
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
         nsIntPoint nsMoveBy = aEvent->refPoint - mLastPoint;

         nsIFrame* parent = GetParent();
         while (parent && parent->GetType() != nsGkAtoms::menuPopupFrame)
           parent = parent->GetParent();

         
         
         if (parent) {
           nsCOMPtr<nsIWidget> widget;
           (static_cast<nsMenuPopupFrame*>(parent))->
             GetWidget(getter_AddRefs(widget));
           nsIntRect bounds;
           widget->GetScreenBounds(bounds);
           widget->Move(bounds.x + nsMoveBy.x, bounds.y + nsMoveBy.y);
         }
         else {
           nsIPresShell* presShell = aPresContext->PresShell();
           nsPIDOMWindow *window = presShell->GetDocument()->GetWindow();
           if (window) {
             window->MoveBy(nsMoveBy.x, nsMoveBy.y);
           }
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
  
  nsContentUtils::DispatchXULCommand(mContent,
                                     aEvent ?
                                       NS_IS_TRUSTED_EVENT(aEvent) : PR_FALSE);
}
