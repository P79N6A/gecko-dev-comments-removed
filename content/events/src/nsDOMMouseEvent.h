





































#ifndef nsDOMMouseEvent_h__
#define nsDOMMouseEvent_h__

#include "nsIDOMMouseEvent.h"
#include "nsDOMUIEvent.h"

class nsIContent;
class nsEvent;

class nsDOMMouseEvent : public nsDOMUIEvent,
                        public nsIDOMMouseEvent
{
public:
  nsDOMMouseEvent(nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMMouseEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMMOUSEEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
protected:
  
  virtual nsresult Which(PRUint32* aWhich);

  nsresult InitMouseEvent(const nsAString& aType,
                          bool aCanBubble,
                          bool aCancelable,
                          nsIDOMWindow* aView,
                          PRInt32 aDetail,
                          PRInt32 aScreenX,
                          PRInt32 aScreenY,
                          PRInt32 aClientX,
                          PRInt32 aClientY,
                          PRUint16 aButton,
                          nsIDOMEventTarget *aRelatedTarget,
                          const nsAString& aModifiersList);
};

#define NS_FORWARD_TO_NSDOMMOUSEEVENT         \
  NS_FORWARD_NSIDOMMOUSEEVENT(nsDOMMouseEvent::) \
  NS_FORWARD_TO_NSDOMUIEVENT

#endif 
