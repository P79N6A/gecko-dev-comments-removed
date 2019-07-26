




#ifndef nsDOMUIEvent_h
#define nsDOMUIEvent_h

#include "nsIDOMUIEvent.h"
#include "nsDOMEvent.h"
#include "nsLayoutUtils.h"
#include "nsEvent.h"

class nsDOMUIEvent : public nsDOMEvent,
                     public nsIDOMUIEvent
{
public:
  nsDOMUIEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMUIEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMUIEVENT
  
  
  NS_FORWARD_TO_NSDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION
  NS_IMETHOD DuplicatePrivateData();
  NS_IMETHOD_(void) Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType);
  NS_IMETHOD_(bool) Deserialize(const IPC::Message* aMsg, void** aIter);

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);

  static nsIntPoint CalculateScreenPoint(nsPresContext* aPresContext,
                                         nsEvent* aEvent)
  {
    if (!aEvent ||
        (aEvent->eventStructType != NS_MOUSE_EVENT &&
         aEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
         aEvent->eventStructType != NS_WHEEL_EVENT &&
         aEvent->eventStructType != NS_DRAG_EVENT &&
         aEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT)) {
      return nsIntPoint(0, 0);
    }

    if (!((nsGUIEvent*)aEvent)->widget ) {
      return aEvent->refPoint;
    }

    nsIntPoint offset = aEvent->refPoint +
                        ((nsGUIEvent*)aEvent)->widget->WidgetToScreenOffset();
    nscoord factor = aPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel();
    return nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(offset.x * factor),
                      nsPresContext::AppUnitsToIntCSSPixels(offset.y * factor));
  }

  static nsIntPoint CalculateClientPoint(nsPresContext* aPresContext,
                                         nsEvent* aEvent,
                                         nsIntPoint* aDefaultClientPoint)
  {
    if (!aEvent ||
        (aEvent->eventStructType != NS_MOUSE_EVENT &&
         aEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
         aEvent->eventStructType != NS_WHEEL_EVENT &&
         aEvent->eventStructType != NS_DRAG_EVENT &&
         aEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT) ||
        !aPresContext ||
        !((nsGUIEvent*)aEvent)->widget) {
      return (nullptr == aDefaultClientPoint ? nsIntPoint(0, 0) :
        nsIntPoint(aDefaultClientPoint->x, aDefaultClientPoint->y));
    }

    nsPoint pt(0, 0);
    nsIPresShell* shell = aPresContext->GetPresShell();
    if (!shell) {
      return nsIntPoint(0, 0);
    }
    nsIFrame* rootFrame = shell->GetRootFrame();
    if (rootFrame) {
      pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, rootFrame);
    }

    return nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(pt.x),
                      nsPresContext::AppUnitsToIntCSSPixels(pt.y));
  }

protected:
  
  nsIntPoint GetClientPoint();
  nsIntPoint GetMovementPoint();
  nsIntPoint GetLayerPoint();
  nsIntPoint GetPagePoint();

  
  virtual nsresult Which(uint32_t* aWhich)
  {
    NS_ENSURE_ARG_POINTER(aWhich);
    
    *aWhich = 0;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindow> mView;
  int32_t mDetail;
  nsIntPoint mClientPoint;
  
  nsIntPoint mLayerPoint;
  nsIntPoint mPagePoint;
  nsIntPoint mMovementPoint;
  bool mIsPointerLocked;
  nsIntPoint mLastClientPoint;

  typedef mozilla::widget::Modifiers Modifiers;
  static Modifiers ComputeModifierState(const nsAString& aModifiersList);
  bool GetModifierStateInternal(const nsAString& aKey);
};

#define NS_FORWARD_TO_NSDOMUIEVENT                          \
  NS_FORWARD_NSIDOMUIEVENT(nsDOMUIEvent::)                  \
  NS_FORWARD_TO_NSDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION  \
  NS_IMETHOD DuplicatePrivateData()                         \
  {                                                         \
    return nsDOMUIEvent::DuplicatePrivateData();            \
  }                                                         \
  NS_IMETHOD_(void) Serialize(IPC::Message* aMsg,           \
                              bool aSerializeInterfaceType) \
  {                                                         \
    nsDOMUIEvent::Serialize(aMsg, aSerializeInterfaceType); \
  }                                                         \
  NS_IMETHOD_(bool) Deserialize(const IPC::Message* aMsg,   \
                                void** aIter)               \
  {                                                         \
    return nsDOMUIEvent::Deserialize(aMsg, aIter);          \
  }

#endif 
