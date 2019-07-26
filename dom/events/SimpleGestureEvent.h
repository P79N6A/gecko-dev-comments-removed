



#ifndef mozilla_dom_SimpleGestureEvent_h_
#define mozilla_dom_SimpleGestureEvent_h_

#include "nsIDOMSimpleGestureEvent.h"
#include "nsDOMMouseEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/SimpleGestureEventBinding.h"

class nsPresContext;

namespace mozilla {
namespace dom {

class SimpleGestureEvent : public nsDOMMouseEvent,
                           public nsIDOMSimpleGestureEvent
{
public:
  SimpleGestureEvent(EventTarget* aOwner,
                     nsPresContext* aPresContext,
                     WidgetSimpleGestureEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMSIMPLEGESTUREEVENT

  
  NS_FORWARD_TO_NSDOMMOUSEEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return SimpleGestureEventBinding::Wrap(aCx, aScope, this);
  }

  uint32_t AllowedDirections();
  uint32_t Direction();
  double Delta();
  uint32_t ClickCount();

  void InitSimpleGestureEvent(const nsAString& aType,
                              bool aCanBubble,
                              bool aCancelable,
                              nsIDOMWindow* aView,
                              int32_t aDetail,
                              int32_t aScreenX,
                              int32_t aScreenY,
                              int32_t aClientX,
                              int32_t aClientY,
                              bool aCtrlKey,
                              bool aAltKey,
                              bool aShiftKey,
                              bool aMetaKey,
                              uint16_t aButton,
                              EventTarget* aRelatedTarget,
                              uint32_t aAllowedDirections,
                              uint32_t aDirection,
                              double aDelta,
                              uint32_t aClickCount,
                              ErrorResult& aRv)
  {
    aRv = InitSimpleGestureEvent(aType, aCanBubble, aCancelable,
                                 aView, aDetail, aScreenX, aScreenY,
                                 aClientX, aClientY, aCtrlKey, aAltKey,
                                 aShiftKey, aMetaKey, aButton,
                                 aRelatedTarget, aAllowedDirections,
                                 aDirection, aDelta, aClickCount);
  }
};

} 
} 

#endif 
