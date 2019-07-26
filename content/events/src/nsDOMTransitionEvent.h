



#ifndef nsDOMTransitionEvent_h_
#define nsDOMTransitionEvent_h_

#include "nsDOMEvent.h"
#include "nsIDOMTransitionEvent.h"
#include "nsString.h"

class nsTransitionEvent;

class nsDOMTransitionEvent : public nsDOMEvent,
                             public nsIDOMTransitionEvent
{
public:
  nsDOMTransitionEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext *aPresContext,
                       nsTransitionEvent *aEvent);
  ~nsDOMTransitionEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMTRANSITIONEVENT

private:
  nsTransitionEvent* TransitionEvent() {
    NS_ABORT_IF_FALSE(mEvent->eventStructType == NS_TRANSITION_EVENT,
                      "unexpected struct type");
    return static_cast<nsTransitionEvent*>(mEvent);
  }
};

#endif 
