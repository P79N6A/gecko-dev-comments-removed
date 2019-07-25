





































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsResizerFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsPIDOMWindow.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsMenuPopupFrame.h"
#include "nsIScreenManager.h"






nsIFrame*
NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsResizerFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsResizerFrame)

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

  switch (aEvent->message) {
    case NS_MOUSE_BUTTON_DOWN: {
      if (aEvent->eventStructType == NS_MOUSE_EVENT &&
        static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton)
      {
        nsCOMPtr<nsIBaseWindow> window;
        nsIPresShell* presShell = aPresContext->GetPresShell();
        nsIContent* contentToResize =
          GetContentToResize(presShell, getter_AddRefs(window));
        if (contentToResize) {
          nsIFrame* frameToResize = contentToResize->GetPrimaryFrame();
          if (!frameToResize)
            break;

          
          
          
          nsRect rect = frameToResize->GetScreenRectInAppUnits();
          switch (frameToResize->GetStylePosition()->mBoxSizing) {
            case NS_STYLE_BOX_SIZING_CONTENT:
              rect.Deflate(frameToResize->GetUsedPadding());
            case NS_STYLE_BOX_SIZING_PADDING:
              rect.Deflate(frameToResize->GetUsedBorder());
            default:
              break;
          }

          mMouseDownRect = rect.ToNearestPixels(aPresContext->AppUnitsPerDevPixel());
        }
        else {
          
          Direction direction = GetDirection();
          nsresult rv = aEvent->widget->BeginResizeDrag(aEvent,
                        direction.mHorizontal, direction.mVertical);
          if (rv == NS_ERROR_NOT_IMPLEMENTED && window) {
            
            
            window->GetPositionAndSize(&mMouseDownRect.x, &mMouseDownRect.y,
                                       &mMouseDownRect.width, &mMouseDownRect.height);
          }
          else {
            
            doDefault = PR_FALSE;
            break;
          }
        }

        
        mTrackingMouseMove = PR_TRUE;

        
        mMouseDownPoint = aEvent->refPoint + aEvent->widget->WidgetToScreenOffset();

        nsIPresShell::SetCapturingContent(GetContent(), CAPTURE_IGNOREALLOWED);

        doDefault = PR_FALSE;
      }
    }
    break;

  case NS_MOUSE_BUTTON_UP: {

    if (mTrackingMouseMove && aEvent->eventStructType == NS_MOUSE_EVENT &&
        static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton)
    {
      
      mTrackingMouseMove = PR_FALSE;

      nsIPresShell::SetCapturingContent(nsnull, 0);

      doDefault = PR_FALSE;
    }
  }
  break;

  case NS_MOUSE_MOVE: {
    if (mTrackingMouseMove)
    {
      nsCOMPtr<nsIBaseWindow> window;
      nsIPresShell* presShell = aPresContext->GetPresShell();
      nsCOMPtr<nsIContent> contentToResize =
        GetContentToResize(presShell, getter_AddRefs(window));

      
      nsMenuPopupFrame* menuPopupFrame = nsnull;
      if (contentToResize) {
        nsIFrame* frameToResize = contentToResize->GetPrimaryFrame();
        if (frameToResize && frameToResize->GetType() == nsGkAtoms::menuPopupFrame) {
          menuPopupFrame = static_cast<nsMenuPopupFrame *>(frameToResize);
        }
      }

      
      

      
      
      nsIntPoint screenPoint(aEvent->refPoint + aEvent->widget->WidgetToScreenOffset());
      nsIntPoint mouseMove(screenPoint - mMouseDownPoint);

      
      
      Direction direction = GetDirection();
      if (window || menuPopupFrame) {
        if (menuPopupFrame) {
          menuPopupFrame->CanAdjustEdges(
            (direction.mHorizontal == -1) ? NS_SIDE_LEFT : NS_SIDE_RIGHT,
            (direction.mVertical == -1) ? NS_SIDE_TOP : NS_SIDE_BOTTOM, mouseMove);
        }
      }
      else if (!contentToResize) {
        break; 
      }

      nsIntRect rect = mMouseDownRect;
      AdjustDimensions(&rect.x, &rect.width, mouseMove.x, direction.mHorizontal);
      AdjustDimensions(&rect.y, &rect.height, mouseMove.y, direction.mVertical);

      
      
      if (window) {
        nsCOMPtr<nsIScreen> screen;
        nsCOMPtr<nsIScreenManager> sm(do_GetService("@mozilla.org/gfx/screenmanager;1"));
        if (sm) {
          nsIntRect frameRect = GetScreenRect();
          sm->ScreenForRect(frameRect.x, frameRect.y, 1, 1, getter_AddRefs(screen));
          if (screen) {
            nsIntRect screenRect;
            screen->GetRect(&screenRect.x, &screenRect.y,
                            &screenRect.width, &screenRect.height);
            rect.IntersectRect(rect, screenRect);
          }
        }
      }
      else if (menuPopupFrame) {
        nsRect frameRect = menuPopupFrame->GetScreenRectInAppUnits();
        nsIFrame* rootFrame = aPresContext->PresShell()->FrameManager()->GetRootFrame();
        nsRect rootScreenRect = rootFrame->GetScreenRectInAppUnits();

        nsRect screenRect = menuPopupFrame->GetConstraintRect(frameRect, rootScreenRect);
        
        
        
        nsIntRect screenRectPixels = screenRect.ToInsidePixels(aPresContext->AppUnitsPerDevPixel());
        rect.IntersectRect(rect, screenRectPixels);
      }

      if (contentToResize) {
        
        
        
        
        nsRect appUnitsRect = rect.ToAppUnits(aPresContext->AppUnitsPerDevPixel());
        if (appUnitsRect.width < mRect.width && mouseMove.x)
          appUnitsRect.width = mRect.width;
        if (appUnitsRect.height < mRect.height && mouseMove.y)
          appUnitsRect.height = mRect.height;
        nsIntRect cssRect = appUnitsRect.ToInsidePixels(nsPresContext::AppUnitsPerCSSPixel());

        nsAutoString widthstr, heightstr;
        widthstr.AppendInt(cssRect.width);
        heightstr.AppendInt(cssRect.height);

        
        
        if (contentToResize->IsXUL()) {
          nsIntRect oldRect;
          nsWeakFrame weakFrame(menuPopupFrame);
          if (menuPopupFrame) {
            nsCOMPtr<nsIWidget> widget;
            menuPopupFrame->GetWidget(getter_AddRefs(widget));
            if (widget)
              widget->GetScreenBounds(oldRect);

            
            nsIntPoint clientOffset = widget->GetClientOffset();
            rect.x -= clientOffset.x; 
            rect.y -= clientOffset.y; 
          }

          
          if (direction.mHorizontal) {
            contentToResize->SetAttr(kNameSpaceID_None, nsGkAtoms::width, widthstr, PR_TRUE);
          }
          if (direction.mVertical) {
            contentToResize->SetAttr(kNameSpaceID_None, nsGkAtoms::height, heightstr, PR_TRUE);
          }

          if (weakFrame.IsAlive() &&
              (oldRect.x != rect.x || oldRect.y != rect.y)) {
            
            
            
            menuPopupFrame->MoveTo(rect.x, rect.y, PR_TRUE);
          }
        }
        else {
          nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyleContent =
            do_QueryInterface(contentToResize);
          if (inlineStyleContent) {
            nsCOMPtr<nsIDOMCSSStyleDeclaration> decl;
            inlineStyleContent->GetStyle(getter_AddRefs(decl));

            
            if (direction.mHorizontal) {
              widthstr.AppendLiteral("px");
              decl->SetProperty(NS_LITERAL_STRING("width"), widthstr, EmptyString());
            }
            if (direction.mVertical) {
              heightstr.AppendLiteral("px");
              decl->SetProperty(NS_LITERAL_STRING("height"), heightstr, EmptyString());
            }
          }
        }
      }
      else {
        window->SetPositionAndSize(rect.x, rect.y, rect.width, rect.height, PR_TRUE); 
      }

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

  if (!doDefault)
    *aEventStatus = nsEventStatus_eConsumeNoDefault;

  if (doDefault && weakFrame.IsAlive())
    return nsTitleBarFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  else
    return NS_OK;
}

nsIContent*
nsResizerFrame::GetContentToResize(nsIPresShell* aPresShell, nsIBaseWindow** aWindow)
{
  *aWindow = nsnull;

  nsAutoString elementid;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::element, elementid);
  if (elementid.IsEmpty()) {
    
    
    nsIFrame* popup = GetParent();
    while (popup) {
      if (popup->GetType() == nsGkAtoms::menuPopupFrame) {
        return popup->GetContent();
      }
      popup = popup->GetParent();
    }

    
    PRBool isChromeShell = PR_FALSE;
    nsCOMPtr<nsISupports> cont = aPresShell->GetPresContext()->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
    if (dsti) {
      PRInt32 type = -1;
      isChromeShell = (NS_SUCCEEDED(dsti->GetItemType(&type)) &&
                       type == nsIDocShellTreeItem::typeChrome);
    }

    if (!isChromeShell) {
      
      
      nsIContent* nonNativeAnon = mContent->FindFirstNonNativeAnonymous();
      if (nonNativeAnon && !nonNativeAnon->GetParent()) {
        return nsnull;
      }
    }

    
    nsPIDOMWindow *domWindow = aPresShell->GetDocument()->GetWindow();
    if (domWindow) {
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
        do_QueryInterface(domWindow->GetDocShell());
      if (docShellAsItem) {
        nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
        docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
        if (treeOwner) {
          CallQueryInterface(treeOwner, aWindow);
        }
      }
    }

    return nsnull;
  }

  if (elementid.EqualsLiteral("_parent")) {
    
    nsIContent* parent = mContent->GetParent();
    return parent ? parent->FindFirstNonNativeAnonymous() : nsnull;
  }

  return aPresShell->GetDocument()->GetElementById(elementid);
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
      
      if (*aSize < 1)
        *aSize = 1;
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
