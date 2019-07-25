





































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
:nsBoxFrame(aPresShell, aContext, false)
{
  mTrackingMouseMove = false;
  UpdateMouseThrough();
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

  bool doDefault = true;

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
             
             mTrackingMouseMove = true;

             
             nsIPresShell::SetCapturingContent(GetContent(), CAPTURE_IGNOREALLOWED);

             
             mLastPoint = aEvent->refPoint;
           }
         }

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = false;
       }
     }
     break;


   case NS_MOUSE_BUTTON_UP: {
       if(mTrackingMouseMove && aEvent->eventStructType == NS_MOUSE_EVENT &&
          static_cast<nsMouseEvent*>(aEvent)->button ==
            nsMouseEvent::eLeftButton)
       {
         
         mTrackingMouseMove = false;

         
         nsIPresShell::SetCapturingContent(nsnull, 0);

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = false;
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
           nsMenuPopupFrame* menuPopupFrame = static_cast<nsMenuPopupFrame*>(parent);
           nsCOMPtr<nsIWidget> widget;
           menuPopupFrame->GetWidget(getter_AddRefs(widget));
           nsIntRect bounds;
           widget->GetScreenBounds(bounds);
           menuPopupFrame->MoveTo(bounds.x + nsMoveBy.x, bounds.y + nsMoveBy.y, false);
         }
         else {
           nsIPresShell* presShell = aPresContext->PresShell();
           nsPIDOMWindow *window = presShell->GetDocument()->GetWindow();
           if (window) {
             window->MoveBy(nsMoveBy.x, nsMoveBy.y);
           }
         }

         *aEventStatus = nsEventStatus_eConsumeNoDefault;

         doDefault = false;
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

void
nsTitleBarFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent)
{
  
  nsContentUtils::DispatchXULCommand(mContent,
                                     aEvent ?
                                       NS_IS_TRUSTED_EVENT(aEvent) : false);
}
