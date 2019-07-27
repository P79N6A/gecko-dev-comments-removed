




#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsResizerFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsPIDOMWindow.h"
#include "mozilla/MouseEvents.h"
#include "nsContentUtils.h"
#include "nsMenuPopupFrame.h"
#include "nsIScreenManager.h"
#include "mozilla/dom/Element.h"
#include "nsError.h"
#include <algorithm>

using namespace mozilla;






nsIFrame*
NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsResizerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsResizerFrame)

nsResizerFrame::nsResizerFrame(nsStyleContext* aContext)
:nsTitleBarFrame(aContext)
{
}

nsresult
nsResizerFrame::HandleEvent(nsPresContext* aPresContext,
                            WidgetGUIEvent* aEvent,
                            nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsWeakFrame weakFrame(this);
  bool doDefault = true;

  switch (aEvent->message) {
    case NS_TOUCH_START:
    case NS_MOUSE_BUTTON_DOWN: {
      if (aEvent->mClass == eTouchEventClass ||
          (aEvent->mClass == eMouseEventClass &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton)) {
        nsCOMPtr<nsIBaseWindow> window;
        nsIPresShell* presShell = aPresContext->GetPresShell();
        nsIContent* contentToResize =
          GetContentToResize(presShell, getter_AddRefs(window));
        if (contentToResize) {
          nsIFrame* frameToResize = contentToResize->GetPrimaryFrame();
          if (!frameToResize)
            break;

          
          
          
          nsRect rect = frameToResize->GetScreenRectInAppUnits();
          switch (frameToResize->StylePosition()->mBoxSizing) {
            case NS_STYLE_BOX_SIZING_CONTENT:
              rect.Deflate(frameToResize->GetUsedPadding());
            case NS_STYLE_BOX_SIZING_PADDING:
              rect.Deflate(frameToResize->GetUsedBorder());
            default:
              break;
          }

          mMouseDownRect =
            LayoutDeviceIntRect::FromAppUnitsToNearest(rect, aPresContext->AppUnitsPerDevPixel());
          doDefault = false;
        }
        else {
          
          if (!window)
            break;

          doDefault = false;
            
          
          Direction direction = GetDirection();
          nsresult rv = aEvent->widget->BeginResizeDrag(aEvent,
                        direction.mHorizontal, direction.mVertical);
          
          if (rv != NS_ERROR_NOT_IMPLEMENTED)
             break;
             
          
          
          window->GetPositionAndSize(&mMouseDownRect.x, &mMouseDownRect.y,
                                     &mMouseDownRect.width, &mMouseDownRect.height);
        }

        
        LayoutDeviceIntPoint refPoint;
        if (!GetEventPoint(aEvent, refPoint))
          return NS_OK;
        mMouseDownPoint = refPoint + aEvent->widget->WidgetToScreenOffset();

        
        mTrackingMouseMove = true;

        nsIPresShell::SetCapturingContent(GetContent(), CAPTURE_IGNOREALLOWED);
      }
    }
    break;

  case NS_TOUCH_END:
  case NS_MOUSE_BUTTON_UP: {
    if (aEvent->mClass == eTouchEventClass ||
        (aEvent->mClass == eMouseEventClass &&
         aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton)) {
      
      mTrackingMouseMove = false;

      nsIPresShell::SetCapturingContent(nullptr, 0);

      doDefault = false;
    }
  }
  break;

  case NS_TOUCH_MOVE:
  case NS_MOUSE_MOVE: {
    if (mTrackingMouseMove)
    {
      nsCOMPtr<nsIBaseWindow> window;
      nsIPresShell* presShell = aPresContext->GetPresShell();
      nsCOMPtr<nsIContent> contentToResize =
        GetContentToResize(presShell, getter_AddRefs(window));

      
      nsMenuPopupFrame* menuPopupFrame = nullptr;
      if (contentToResize) {
        menuPopupFrame = do_QueryFrame(contentToResize->GetPrimaryFrame());
      }

      
      

      
      
      LayoutDeviceIntPoint refPoint;
      if (!GetEventPoint(aEvent, refPoint))
        return NS_OK;
      LayoutDeviceIntPoint screenPoint = refPoint + aEvent->widget->WidgetToScreenOffset();
      LayoutDeviceIntPoint mouseMove(screenPoint - mMouseDownPoint);

      
      
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

      LayoutDeviceIntRect rect = mMouseDownRect;

      
      widget::SizeConstraints sizeConstraints;
      if (window) {
        nsCOMPtr<nsIWidget> widget;
        window->GetMainWidget(getter_AddRefs(widget));
        sizeConstraints = widget->GetSizeConstraints();
      }

      AdjustDimensions(&rect.x, &rect.width, sizeConstraints.mMinSize.width,
                       sizeConstraints.mMaxSize.width, mouseMove.x, direction.mHorizontal);
      AdjustDimensions(&rect.y, &rect.height, sizeConstraints.mMinSize.height,
                       sizeConstraints.mMaxSize.height, mouseMove.y, direction.mVertical);

      
      
      if (window) {
        nsCOMPtr<nsIScreen> screen;
        nsCOMPtr<nsIScreenManager> sm(do_GetService("@mozilla.org/gfx/screenmanager;1"));
        if (sm) {
          nsIntRect frameRect = GetScreenRect();
          
          double scale;
          window->GetUnscaledDevicePixelsPerCSSPixel(&scale);
          sm->ScreenForRect(NSToIntRound(frameRect.x / scale),
                            NSToIntRound(frameRect.y / scale), 1, 1,
                            getter_AddRefs(screen));
          if (screen) {
            LayoutDeviceIntRect screenRect;
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

        nsPopupLevel popupLevel = menuPopupFrame->PopupLevel();
        nsRect screenRect = menuPopupFrame->GetConstraintRect(frameRect, rootScreenRect, popupLevel);
        
        
        
        nsIntRect screenRectPixels = screenRect.ToInsidePixels(aPresContext->AppUnitsPerDevPixel());
        rect.IntersectRect(rect, LayoutDevicePixel::FromUntyped(screenRectPixels));
      }

      if (contentToResize) {
        
        
        
        
        nsRect appUnitsRect = ToAppUnits(LayoutDevicePixel::ToUntyped(rect), aPresContext->AppUnitsPerDevPixel());
        if (appUnitsRect.width < mRect.width && mouseMove.x)
          appUnitsRect.width = mRect.width;
        if (appUnitsRect.height < mRect.height && mouseMove.y)
          appUnitsRect.height = mRect.height;
        nsIntRect cssRect = appUnitsRect.ToInsidePixels(nsPresContext::AppUnitsPerCSSPixel());

        nsIntRect oldRect;
        nsWeakFrame weakFrame(menuPopupFrame);
        if (menuPopupFrame) {
          nsCOMPtr<nsIWidget> widget = menuPopupFrame->GetWidget();
          if (widget)
            widget->GetScreenBounds(oldRect);

          
          nsIntPoint clientOffset = widget->GetClientOffset();
          rect.x -= clientOffset.x;
          rect.y -= clientOffset.y;
        }

        SizeInfo sizeInfo, originalSizeInfo;
        sizeInfo.width.AppendInt(cssRect.width);
        sizeInfo.height.AppendInt(cssRect.height);
        ResizeContent(contentToResize, direction, sizeInfo, &originalSizeInfo);
        MaybePersistOriginalSize(contentToResize, originalSizeInfo);

        
        
        
        
        if (weakFrame.IsAlive() &&
            (oldRect.x != rect.x || oldRect.y != rect.y) &&
            (!menuPopupFrame->IsAnchored() ||
             menuPopupFrame->PopupLevel() != ePopupLevelParent)) {

          rect.x = aPresContext->DevPixelsToIntCSSPixels(rect.x);
          rect.y = aPresContext->DevPixelsToIntCSSPixels(rect.y);
          menuPopupFrame->MoveTo(rect.x, rect.y, true);
        }
      }
      else {
        window->SetPositionAndSize(rect.x, rect.y, rect.width, rect.height, true); 
      }

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
  case NS_MOUSE_DOUBLECLICK:
    if (aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton) {
      nsCOMPtr<nsIBaseWindow> window;
      nsIPresShell* presShell = aPresContext->GetPresShell();
      nsIContent* contentToResize =
        GetContentToResize(presShell, getter_AddRefs(window));
      if (contentToResize) {
        nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(contentToResize->GetPrimaryFrame());
        if (menuPopupFrame)
          break; 
                 

        RestoreOriginalSize(contentToResize);
      }
    }
    break;
  }

  if (!doDefault)
    *aEventStatus = nsEventStatus_eConsumeNoDefault;

  if (doDefault && weakFrame.IsAlive())
    return nsTitleBarFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  return NS_OK;
}

nsIContent*
nsResizerFrame::GetContentToResize(nsIPresShell* aPresShell, nsIBaseWindow** aWindow)
{
  *aWindow = nullptr;

  nsAutoString elementid;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::element, elementid);
  if (elementid.IsEmpty()) {
    
    
    nsIFrame* popup = GetParent();
    while (popup) {
      nsMenuPopupFrame* popupFrame = do_QueryFrame(popup);
      if (popupFrame) {
        return popupFrame->GetContent();
      }
      popup = popup->GetParent();
    }

    
    nsCOMPtr<nsIDocShellTreeItem> dsti = aPresShell->GetPresContext()->GetDocShell();
    if (!dsti || dsti->ItemType() != nsIDocShellTreeItem::typeChrome) {
      
      
      nsIContent* nonNativeAnon = mContent->FindFirstNonChromeOnlyAccessContent();
      if (!nonNativeAnon || nonNativeAnon->GetParent()) {
        return nullptr;
      }
    }

    
    nsPIDOMWindow *domWindow = aPresShell->GetDocument()->GetWindow();
    if (domWindow) {
      nsCOMPtr<nsIDocShell> docShell = domWindow->GetDocShell();
      if (docShell) {
        nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
        docShell->GetTreeOwner(getter_AddRefs(treeOwner));
        if (treeOwner) {
          CallQueryInterface(treeOwner, aWindow);
        }
      }
    }

    return nullptr;
  }

  if (elementid.EqualsLiteral("_parent")) {
    
    nsIContent* parent = mContent->GetParent();
    return parent ? parent->FindFirstNonChromeOnlyAccessContent() : nullptr;
  }

  return aPresShell->GetDocument()->GetElementById(elementid);
}

void
nsResizerFrame::AdjustDimensions(int32_t* aPos, int32_t* aSize,
                                 int32_t aMinSize, int32_t aMaxSize,
                                 int32_t aMovement, int8_t aResizerDirection)
{
  int32_t oldSize = *aSize;

  *aSize += aResizerDirection * aMovement;
  
  if (*aSize < 1)
    *aSize = 1;

  
  *aSize = std::max(aMinSize, std::min(aMaxSize, *aSize));

  
  
  if (aResizerDirection == -1)
    *aPos += oldSize - *aSize;
}

 void
nsResizerFrame::ResizeContent(nsIContent* aContent, const Direction& aDirection,
                              const SizeInfo& aSizeInfo, SizeInfo* aOriginalSizeInfo)
{
  
  
  if (aContent->IsXULElement()) {
    if (aOriginalSizeInfo) {
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::width,
                        aOriginalSizeInfo->width);
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::height,
                        aOriginalSizeInfo->height);
    }
    
    if (aDirection.mHorizontal) {
      aContent->SetAttr(kNameSpaceID_None, nsGkAtoms::width, aSizeInfo.width, true);
    }
    if (aDirection.mVertical) {
      aContent->SetAttr(kNameSpaceID_None, nsGkAtoms::height, aSizeInfo.height, true);
    }
  }
  else {
    nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyleContent =
      do_QueryInterface(aContent);
    if (inlineStyleContent) {
      nsCOMPtr<nsIDOMCSSStyleDeclaration> decl;
      inlineStyleContent->GetStyle(getter_AddRefs(decl));

      if (aOriginalSizeInfo) {
        decl->GetPropertyValue(NS_LITERAL_STRING("width"),
                               aOriginalSizeInfo->width);
        decl->GetPropertyValue(NS_LITERAL_STRING("height"),
                               aOriginalSizeInfo->height);
      }

      
      if (aDirection.mHorizontal) {
        nsAutoString widthstr(aSizeInfo.width);
        if (!widthstr.IsEmpty() &&
            !Substring(widthstr, widthstr.Length() - 2, 2).EqualsLiteral("px"))
          widthstr.AppendLiteral("px");
        decl->SetProperty(NS_LITERAL_STRING("width"), widthstr, EmptyString());
      }
      if (aDirection.mVertical) {
        nsAutoString heightstr(aSizeInfo.height);
        if (!heightstr.IsEmpty() &&
            !Substring(heightstr, heightstr.Length() - 2, 2).EqualsLiteral("px"))
          heightstr.AppendLiteral("px");
        decl->SetProperty(NS_LITERAL_STRING("height"), heightstr, EmptyString());
      }
    }
  }
}

 void
nsResizerFrame::MaybePersistOriginalSize(nsIContent* aContent,
                                         const SizeInfo& aSizeInfo)
{
  nsresult rv;

  aContent->GetProperty(nsGkAtoms::_moz_original_size, &rv);
  if (rv != NS_PROPTABLE_PROP_NOT_THERE)
    return;

  nsAutoPtr<SizeInfo> sizeInfo(new SizeInfo(aSizeInfo));
  rv = aContent->SetProperty(nsGkAtoms::_moz_original_size, sizeInfo.get(),
                             nsINode::DeleteProperty<nsResizerFrame::SizeInfo>);
  if (NS_SUCCEEDED(rv))
    sizeInfo.forget();
}

 void
nsResizerFrame::RestoreOriginalSize(nsIContent* aContent)
{
  nsresult rv;
  SizeInfo* sizeInfo =
    static_cast<SizeInfo*>(aContent->GetProperty(nsGkAtoms::_moz_original_size,
                           &rv));
  if (NS_FAILED(rv))
    return;

  NS_ASSERTION(sizeInfo, "We set a null sizeInfo!?");
  Direction direction = {1, 1};
  ResizeContent(aContent, direction, *sizeInfo, nullptr);
  aContent->DeleteProperty(nsGkAtoms::_moz_original_size);
}



nsResizerFrame::Direction
nsResizerFrame::GetDirection()
{
  static const nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::topleft,    &nsGkAtoms::top,    &nsGkAtoms::topright,
     &nsGkAtoms::left,                           &nsGkAtoms::right,
     &nsGkAtoms::bottomleft, &nsGkAtoms::bottom, &nsGkAtoms::bottomright,
     &nsGkAtoms::bottomstart,                    &nsGkAtoms::bottomend,
     nullptr};

  static const Direction directions[] =
    {{-1, -1}, {0, -1}, {1, -1},
     {-1,  0},          {1,  0},
     {-1,  1}, {0,  1}, {1,  1},
     {-1,  1},          {1,  1}
    };

  if (!GetContent()) {
    return directions[0]; 
  }

  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsGkAtoms::dir,
                                                strings, eCaseMatters);
  if (index < 0) {
    return directions[0]; 
  }

  if (index >= 8) {
    
    
    WritingMode wm = GetWritingMode();
    if (!(wm.IsVertical() ? wm.IsVerticalLR() : wm.IsBidiLTR())) {
      Direction direction = directions[index];
      direction.mHorizontal *= -1;
      return direction;
    }
  }

  return directions[index];
}

void
nsResizerFrame::MouseClicked(nsPresContext* aPresContext,
                             WidgetMouseEvent* aEvent)
{
  
  nsContentUtils::DispatchXULCommand(mContent,
                                     aEvent && aEvent->mFlags.mIsTrusted);
}
