




#ifndef nsDOMMouseEvent_h__
#define nsDOMMouseEvent_h__

#include "nsIDOMMouseEvent.h"
#include "nsDOMUIEvent.h"

class nsEvent;

class nsDOMMouseEvent : public nsDOMUIEvent,
                        public nsIDOMMouseEvent
{
public:
  nsDOMMouseEvent(mozilla::dom::EventTarget* aOwner,
                  nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMMouseEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMMOUSEEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
protected:
  
  virtual nsresult Which(uint32_t* aWhich);

  nsresult InitMouseEvent(const nsAString& aType,
                          bool aCanBubble,
                          bool aCancelable,
                          nsIDOMWindow* aView,
                          int32_t aDetail,
                          int32_t aScreenX,
                          int32_t aScreenY,
                          int32_t aClientX,
                          int32_t aClientY,
                          uint16_t aButton,
                          nsIDOMEventTarget *aRelatedTarget,
                          const nsAString& aModifiersList);
};

#define NS_FORWARD_TO_NSDOMMOUSEEVENT         \
  NS_FORWARD_NSIDOMMOUSEEVENT(nsDOMMouseEvent::) \
  NS_FORWARD_TO_NSDOMUIEVENT

#endif 
