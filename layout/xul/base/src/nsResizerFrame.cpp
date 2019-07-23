





































#include "nsCOMPtr.h"
#include "nsResizerFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsPIDOMWindow.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"






nsIFrame*
NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsResizerFrame(aPresShell, aContext);
} 

nsResizerFrame::nsResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
:nsTitleBarFrame(aPresShell, aContext)
{
  mDirection = topleft; 
}

NS_IMETHODIMP
nsResizerFrame::Init(nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIFrame*        asPrevInFlow)
{
  nsresult rv = nsTitleBarFrame::Init(aContent, aParent, asPrevInFlow);

  GetInitialDirection(mDirection);

  return rv;
}


NS_IMETHODIMP
nsResizerFrame::HandleEvent(nsPresContext* aPresContext,
                            nsGUIEvent* aEvent,
                            nsEventStatus* aEventStatus)
{
  nsWeakFrame weakFrame(this);
  PRBool doDefault = PR_TRUE;

  switch (aEvent->message) {

   case NS_MOUSE_BUTTON_DOWN: {
       if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
             nsMouseEvent::eLeftButton)
       {
         
         mTrackingMouseMove = PR_TRUE;

         
         aEvent->widget->CaptureMouse(PR_TRUE);
         CaptureMouseEvents(aPresContext,PR_TRUE);

         
         mLastPoint = aEvent->refPoint;
         aEvent->widget->GetScreenBounds(mWidgetRect);

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

         nsPoint nsMoveBy(0,0),nsSizeBy(0,0);
         nsPoint nsMouseMove(aEvent->refPoint - mLastPoint);

         switch(mDirection)
         {
            case topleft:
              nsMoveBy = nsMouseMove;
              nsSizeBy -= nsMouseMove;
              break;
            case top:
              nsMoveBy.y = nsMouseMove.y;
              nsSizeBy.y = - nsMouseMove.y;
              break;
            case topright:
              nsMoveBy.y = nsMouseMove.y;
              nsSizeBy.x = nsMouseMove.x;
              mLastPoint.x += nsMouseMove.x;
              nsSizeBy.y = -nsMouseMove.y;
              break;
            case left:
              nsMoveBy.x = nsMouseMove.x;
              nsSizeBy.x = -nsMouseMove.x;
              break;
            case right:
              nsSizeBy.x = nsMouseMove.x;
              mLastPoint.x += nsMouseMove.x;
              break;
            case bottomleft:
              nsMoveBy.x = nsMouseMove.x;
              nsSizeBy.y = nsMouseMove.y;
              nsSizeBy.x = -nsMouseMove.x;
              mLastPoint.y += nsMouseMove.y;
              break;
            case bottom:
              nsSizeBy.y = nsMouseMove.y;
              mLastPoint.y += nsMouseMove.y;
              break;
            case bottomright:
              nsSizeBy = nsMouseMove;
              mLastPoint += nsMouseMove;
              break;
         }

         PRInt32 x,y,cx,cy;
         window->GetPositionAndSize(&x,&y,&cx,&cy);

         x+=nsMoveBy.x;
         y+=nsMoveBy.y;
         cx+=nsSizeBy.x;
         cy+=nsSizeBy.y;

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





PRBool
nsResizerFrame::EvalDirection(nsAutoString& aText,eDirection& aDir)
{
  PRBool aResult = PR_TRUE;

  if( aText.Equals( NS_LITERAL_STRING("topleft") ) )
  {
    aDir = topleft;
  }
  else if( aText.Equals( NS_LITERAL_STRING("top") ) )
  {
    aDir = top;
  }
  else if( aText.Equals( NS_LITERAL_STRING("topright") ) )
  {
    aDir = topright;
  }
  else if( aText.Equals( NS_LITERAL_STRING("left") ) )
  {
    aDir = left;
  }
  else if( aText.Equals( NS_LITERAL_STRING("right") ) )
  {
    aDir = right;
  }
  else if( aText.Equals( NS_LITERAL_STRING("bottomleft") ) )
  {
    aDir = bottomleft;
  }
  else if( aText.Equals( NS_LITERAL_STRING("bottom") ) )
  {
    aDir = bottom;
  }
  else if( aText.Equals( NS_LITERAL_STRING("bottomright") ) )
  {
    aDir = bottomright;
  }
  else
  {
    aResult = PR_FALSE;
  }

  return aResult;
}




PRBool
nsResizerFrame::GetInitialDirection(eDirection& aDirection)
{
 
  nsAutoString value;

  if (!GetContent())
     return PR_FALSE;

  if (GetContent()->GetAttr(kNameSpaceID_None, nsGkAtoms::dir, value)) {
     return EvalDirection(value,aDirection);
  }

  return PR_FALSE;
}


NS_IMETHODIMP
nsResizerFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 PRInt32 aModType)
{
  nsresult rv = nsTitleBarFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                                  aModType);

  if (aAttribute == nsGkAtoms::dir) {
    GetInitialDirection(mDirection);
  }

  return rv;
}



void
nsResizerFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent *aEvent)
{
  
  nsEventStatus status = nsEventStatus_eIgnore;

  nsXULCommandEvent event(aEvent ? NS_IS_TRUSTED_EVENT(aEvent) : PR_FALSE,
                          NS_XUL_COMMAND, nsnull);

  nsEventDispatcher::Dispatch(mContent, aPresContext, &event, nsnull, &status);
}
