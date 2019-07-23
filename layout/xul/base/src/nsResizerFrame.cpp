





































#include "nsCOMPtr.h"
#include "nsResizerFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsPIDOMWindow.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"






nsIFrame*
NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsResizerFrame(aPresShell, aContext);
} 

nsResizerFrame::nsResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
:nsTitleBarFrame(aPresShell, aContext)
{
}

NS_IMETHODIMP
nsResizerFrame::HandleEvent(nsPresContext* aPresContext,
                            nsGUIEvent* aEvent,
                            nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsWeakFrame weakFrame(this);
  PRBool doDefault = PR_TRUE;

  
  Direction direction = GetDirection();

  switch (aEvent->message) {

   case NS_MOUSE_BUTTON_DOWN: {
       if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           static_cast<nsMouseEvent*>(aEvent)->button ==
             nsMouseEvent::eLeftButton)
       {

         nsresult rv = NS_OK;

         
         rv = aEvent->widget->BeginResizeDrag(aEvent, 
             direction.mHorizontal, direction.mVertical);

         if (rv == NS_ERROR_NOT_IMPLEMENTED) {
           
           

           
           mTrackingMouseMove = PR_TRUE;

           
           aEvent->widget->CaptureMouse(PR_TRUE);
           CaptureMouseEvents(aPresContext,PR_TRUE);

           
           mLastPoint = aEvent->refPoint;
           aEvent->widget->GetScreenBounds(mWidgetRect);
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

         
         aEvent->widget->CaptureMouse(PR_FALSE);
         CaptureMouseEvents(aPresContext,PR_FALSE);

         *aEventStatus = nsEventStatus_eConsumeNoDefault;
         doDefault = PR_FALSE;
       }
     }
     break;

   case NS_MOUSE_MOVE: {
       if(mTrackingMouseMove)
       {
         
         nsPIDOMWindow *domWindow =
           aPresContext->PresShell()->GetDocument()->GetWindow();
         NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);

         nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
           do_QueryInterface(domWindow->GetDocShell());
         NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

         nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
         docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));

         nsCOMPtr<nsIBaseWindow> window(do_QueryInterface(treeOwner));

         if (!window) {
           return NS_OK;
         }

         PRInt32 x,y,cx,cy;
         window->GetPositionAndSize(&x,&y,&cx,&cy);
         nsIntPoint oldWindowTopLeft(x, y);

         
         
         nsIntPoint mouseMove(aEvent->refPoint - mLastPoint);
         
         AdjustDimensions(&x, &cx, mouseMove.x, direction.mHorizontal);
         AdjustDimensions(&y, &cy, mouseMove.y, direction.mVertical);

         
         mLastPoint = aEvent->refPoint + (oldWindowTopLeft - nsIntPoint(x, y));

         window->SetPositionAndSize(x,y,cx,cy,PR_TRUE); 

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

  if (doDefault && weakFrame.IsAlive())
    return nsTitleBarFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  else
    return NS_OK;
}




void
nsResizerFrame::AdjustDimensions(PRInt32* aPos, PRInt32* aSize,
                                 PRInt32 aMovement, PRInt8 aResizerDirection)
{
  switch(aResizerDirection)
  {
    case -1: 
      *aPos+= aMovement;
    case 1: 
      *aSize+= aResizerDirection*aMovement;
  }
}



nsResizerFrame::Direction
nsResizerFrame::GetDirection()
{
  static const nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::topleft,    &nsGkAtoms::top,    &nsGkAtoms::topright,
     &nsGkAtoms::left,                           &nsGkAtoms::right,
     &nsGkAtoms::bottomleft, &nsGkAtoms::bottom, &nsGkAtoms::bottomright,
                                                 &nsGkAtoms::bottomend,
     nsnull};

  static const Direction directions[] =
    {{-1, -1}, {0, -1}, {1, -1},
     {-1,  0},          {1,  0},
     {-1,  1}, {0,  1}, {1,  1},
                        {1,  1}
    };

  if (!GetContent())
    return directions[0]; 

  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsGkAtoms::dir,
                                                strings, eCaseMatters);
  if(index < 0)
    return directions[0]; 
  else if (index >= 8 && GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    
    
    Direction direction = directions[index];
    direction.mHorizontal *= -1;
    return direction;
  }
  return directions[index];
}

void
nsResizerFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent *aEvent)
{
  
  nsContentUtils::DispatchXULCommand(mContent,
                                     aEvent ?
                                       NS_IS_TRUSTED_EVENT(aEvent) : PR_FALSE);
}
