






































#include "base/basictypes.h"
#include "IPC/IPCMessageUtils.h"
#include "nsCOMPtr.h"
#include "nsDOMUIEvent.h"
#include "nsIPresShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsEventStateManager.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "DictionaryHelpers.h"

nsDOMUIEvent::nsDOMUIEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ?
               static_cast<nsEvent *>(aEvent) :
               static_cast<nsEvent *>(new nsUIEvent(false, 0, 0)))
  , mClientPoint(0, 0), mLayerPoint(0, 0), mPagePoint(0, 0)
  , mIsPointerLocked(nsEventStateManager::sIsPointerLocked)
  , mLastScreenPoint(nsEventStateManager::sLastScreenPoint)
  , mLastClientPoint(nsEventStateManager::sLastClientPoint)
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
  
  
  
  switch(mEvent->eventStructType)
  {
    case NS_UI_EVENT:
    {
      nsUIEvent *event = static_cast<nsUIEvent*>(mEvent);
      mDetail = event->detail;
      break;
    }

    case NS_SCROLLPORT_EVENT:
    {
      nsScrollPortEvent* scrollEvent = static_cast<nsScrollPortEvent*>(mEvent);
      mDetail = (PRInt32)scrollEvent->orient;
      break;
    }

    default:
      mDetail = 0;
      break;
  }

  mView = nsnull;
  if (mPresContext)
  {
    nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
    if (container)
    {
       nsCOMPtr<nsIDOMWindow> window = do_GetInterface(container);
       if (window)
          mView = do_QueryInterface(window);
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMUIEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMUIEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMUIEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsDOMUIEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMUIEvent, nsDOMEvent)

DOMCI_DATA(UIEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMUIEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUIEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(UIEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsIntPoint
nsDOMUIEvent::GetMovementPoint()
{
  if (!mEvent ||
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        mEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
        mEvent->eventStructType != NS_MOZTOUCH_EVENT &&
        mEvent->eventStructType != NS_DRAG_EVENT &&
        mEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT)) {
    return nsIntPoint(0, 0);
  }

  if (!((nsGUIEvent*)mEvent)->widget) {
    return mEvent->lastRefPoint;
  }

  
  nsIntPoint currentPoint = CalculateScreenPoint(mPresContext, mEvent);

  
  nsIntPoint offset = mEvent->lastRefPoint +
    ((nsGUIEvent*)mEvent)->widget->WidgetToScreenOffset();
  nscoord factor = mPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel();
  nsIntPoint lastPoint = nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(offset.x * factor),
                                    nsPresContext::AppUnitsToIntCSSPixels(offset.y * factor));

  return currentPoint - lastPoint;
}

nsIntPoint
nsDOMUIEvent::GetScreenPoint()
{
  if (mIsPointerLocked) {
    return mLastScreenPoint;
  }

  return CalculateScreenPoint(mPresContext, mEvent);
}

nsIntPoint
nsDOMUIEvent::GetClientPoint()
{
  if (mIsPointerLocked) {
    return mLastClientPoint;
  }

  return CalculateClientPoint(mPresContext, mEvent, &mClientPoint);
}

NS_IMETHODIMP
nsDOMUIEvent::GetView(nsIDOMWindow** aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetDetail(PRInt32* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::InitUIEvent(const nsAString& typeArg,
                          bool canBubbleArg,
                          bool cancelableArg,
                          nsIDOMWindow* viewArg,
                          PRInt32 detailArg)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mDetail = detailArg;
  mView = viewArg;

  return NS_OK;
}

nsresult
nsDOMUIEvent::InitFromCtor(const nsAString& aType,
                           JSContext* aCx, jsval* aVal)
{
  mozilla::dom::UIEventInit d;
  nsresult rv = d.Init(aCx, aVal);
  NS_ENSURE_SUCCESS(rv, rv);
  return InitUIEvent(aType, d.bubbles, d.cancelable, d.view, d.detail);
}


nsIntPoint
nsDOMUIEvent::GetPagePoint()
{
  if (mPrivateDataDuplicated) {
    return mPagePoint;
  }

  nsIntPoint pagePoint = GetClientPoint();

  
  if (mPresContext && mPresContext->GetPresShell()) {
    nsIPresShell* shell = mPresContext->GetPresShell();
    nsIScrollableFrame* scrollframe = shell->GetRootScrollFrameAsScrollable();
    if (scrollframe) {
      nsPoint pt = scrollframe->GetScrollPosition();
      pagePoint += nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(pt.x),
                              nsPresContext::AppUnitsToIntCSSPixels(pt.y));
    }
  }

  return pagePoint;
}

NS_IMETHODIMP
nsDOMUIEvent::GetPageX(PRInt32* aPageX)
{
  NS_ENSURE_ARG_POINTER(aPageX);
  if (mPrivateDataDuplicated) {
    *aPageX = mPagePoint.x;
  } else {
    *aPageX = nsDOMEvent::GetPageCoords(mPresContext,
                                        mEvent,
                                        mEvent->refPoint,
                                        mClientPoint).x;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetPageY(PRInt32* aPageY)
{
  NS_ENSURE_ARG_POINTER(aPageY);
  if (mPrivateDataDuplicated) {
    *aPageY = mPagePoint.y;
  } else {
    *aPageY = nsDOMEvent::GetPageCoords(mPresContext,
                                        mEvent,
                                        mEvent->refPoint,
                                        mClientPoint).y;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetWhich(PRUint32* aWhich)
{
  return Which(aWhich);
}

NS_IMETHODIMP
nsDOMUIEvent::GetRangeParent(nsIDOMNode** aRangeParent)
{
  NS_ENSURE_ARG_POINTER(aRangeParent);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  }

  *aRangeParent = nsnull;

  if (targetFrame) {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent,
                                                              targetFrame);
    nsCOMPtr<nsIContent> parent = targetFrame->GetContentOffsetsFromPoint(pt).content;
    if (parent) {
      if (parent->IsInNativeAnonymousSubtree() &&
          !nsContentUtils::CanAccessNativeAnon()) {
        return NS_OK;
      }
      return CallQueryInterface(parent, aRangeParent);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetRangeOffset(PRInt32* aRangeOffset)
{
  NS_ENSURE_ARG_POINTER(aRangeOffset);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  }

  if (targetFrame) {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent,
                                                              targetFrame);
    *aRangeOffset = targetFrame->GetContentOffsetsFromPoint(pt).offset;
    return NS_OK;
  }
  *aRangeOffset = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetCancelBubble(bool* aCancelBubble)
{
  NS_ENSURE_ARG_POINTER(aCancelBubble);
  *aCancelBubble =
    (mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) ? true : false;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::SetCancelBubble(bool aCancelBubble)
{
  if (aCancelBubble) {
    mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  } else {
    mEvent->flags &= ~NS_EVENT_FLAG_STOP_DISPATCH;
  }
  return NS_OK;
}

nsIntPoint
nsDOMUIEvent::GetLayerPoint()
{
  if (!mEvent ||
      (mEvent->eventStructType != NS_MOUSE_EVENT &&
       mEvent->eventStructType != NS_POPUP_EVENT &&
       mEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
       mEvent->eventStructType != NS_MOZTOUCH_EVENT &&
       mEvent->eventStructType != NS_TOUCH_EVENT &&
       mEvent->eventStructType != NS_DRAG_EVENT &&
       mEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT) ||
      !mPresContext ||
      mEventIsInternal) {
    return mLayerPoint;
  }
  
  nsIFrame* targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  if (!targetFrame)
    return mLayerPoint;
  nsIFrame* layer = nsLayoutUtils::GetClosestLayer(targetFrame);
  nsPoint pt(nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent, layer));
  return nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(pt.x),
                    nsPresContext::AppUnitsToIntCSSPixels(pt.y));
}

NS_IMETHODIMP
nsDOMUIEvent::GetLayerX(PRInt32* aLayerX)
{
  NS_ENSURE_ARG_POINTER(aLayerX);
  *aLayerX = GetLayerPoint().x;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetLayerY(PRInt32* aLayerY)
{
  NS_ENSURE_ARG_POINTER(aLayerY);
  *aLayerY = GetLayerPoint().y;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetIsChar(bool* aIsChar)
{
  switch(mEvent->eventStructType)
  {
    case NS_KEY_EVENT:
      *aIsChar = ((nsKeyEvent*)mEvent)->isChar;
      return NS_OK;
    case NS_TEXT_EVENT:
      *aIsChar = ((nsTextEvent*)mEvent)->isChar;
      return NS_OK;
    default:
      *aIsChar = false;
      return NS_OK;
  }
}

NS_METHOD
nsDOMUIEvent::DuplicatePrivateData()
{
  mClientPoint = nsDOMEvent::GetClientCoords(mPresContext,
                                             mEvent,
                                             mEvent->refPoint,
                                             mClientPoint);
  mLayerPoint = GetLayerPoint();
  mPagePoint = nsDOMEvent::GetPageCoords(mPresContext,
                                         mEvent,
                                         mEvent->refPoint,
                                         mClientPoint);
  
  nsIntPoint screenPoint = nsDOMEvent::GetScreenCoords(mPresContext,
                                                       mEvent,
                                                       mEvent->refPoint);
  nsresult rv = nsDOMEvent::DuplicatePrivateData();
  if (NS_SUCCEEDED(rv)) {
    mEvent->refPoint = screenPoint;
  }
  return rv;
}

void
nsDOMUIEvent::Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("uievent"));
  }

  nsDOMEvent::Serialize(aMsg, false);

  PRInt32 detail = 0;
  GetDetail(&detail);
  IPC::WriteParam(aMsg, detail);
}

bool
nsDOMUIEvent::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  NS_ENSURE_TRUE(nsDOMEvent::Deserialize(aMsg, aIter), false);
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &mDetail), false);
  return true;
}

nsresult NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult,
                          nsPresContext* aPresContext,
                          nsGUIEvent *aEvent) 
{
  nsDOMUIEvent* it = new nsDOMUIEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
