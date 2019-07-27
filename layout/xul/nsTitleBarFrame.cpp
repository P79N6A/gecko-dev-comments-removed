




#include "nsCOMPtr.h"
#include "nsTitleBarFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsIWidget.h"
#include "nsMenuPopupFrame.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"
#include "mozilla/MouseEvents.h"

using namespace mozilla;






nsIFrame*
NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTitleBarFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTitleBarFrame)

nsTitleBarFrame::nsTitleBarFrame(nsStyleContext* aContext)
:nsBoxFrame(aContext, false)
{
  mTrackingMouseMove = false;
  UpdateMouseThrough();
}

void
nsTitleBarFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  
  if (aBuilder->IsForEventDelivery()) {
    if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::allowevents,
                               nsGkAtoms::_true, eCaseMatters))
      return;
  }
  nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

nsresult
nsTitleBarFrame::HandleEvent(nsPresContext* aPresContext,
                             WidgetGUIEvent* aEvent,
                             nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  bool doDefault = true;

  switch (aEvent->message) {

   case NS_MOUSE_BUTTON_DOWN:  {
       if (aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton) {
         
         nsCOMPtr<nsIDocShellTreeItem> dsti = aPresContext->GetDocShell();
         if (dsti) {
           if (dsti->ItemType() == nsIDocShellTreeItem::typeChrome) {
             
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
       if (mTrackingMouseMove &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton) {
         
         mTrackingMouseMove = false;

         
         nsIPresShell::SetCapturingContent(nullptr, 0);

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = false;
       }
     }
     break;

   case NS_MOUSE_MOVE: {
       if(mTrackingMouseMove)
       {
         LayoutDeviceIntPoint nsMoveBy = aEvent->refPoint - mLastPoint;

         nsIFrame* parent = GetParent();
         while (parent) {
           nsMenuPopupFrame* popupFrame = do_QueryFrame(parent);
           if (popupFrame)
             break;
           parent = parent->GetParent();
         }

         
         
         if (parent) {
           nsMenuPopupFrame* menuPopupFrame = static_cast<nsMenuPopupFrame*>(parent);
           nsCOMPtr<nsIWidget> widget = menuPopupFrame->GetWidget();
           nsIntRect bounds;
           widget->GetScreenBounds(bounds);

           int32_t newx = aPresContext->DevPixelsToIntCSSPixels(bounds.x + nsMoveBy.x);
           int32_t newy = aPresContext->DevPixelsToIntCSSPixels(bounds.y + nsMoveBy.y);
           menuPopupFrame->MoveTo(newx, newy, false);
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

    case NS_MOUSE_CLICK: {
      WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
      if (mouseEvent->IsLeftClickEvent()) {
        MouseClicked(aPresContext, mouseEvent);
      }
      break;
    }
  }

  if ( doDefault )
    return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  else
    return NS_OK;
}

void
nsTitleBarFrame::MouseClicked(nsPresContext* aPresContext,
                              WidgetMouseEvent* aEvent)
{
  
  nsContentUtils::DispatchXULCommand(mContent,
                                     aEvent && aEvent->mFlags.mIsTrusted);
}
