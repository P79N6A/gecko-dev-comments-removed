







#ifndef nsDOMXULCommandEvent_h_
#define nsDOMXULCommandEvent_h_

#include "nsDOMUIEvent.h"
#include "nsIDOMXULCommandEvent.h"

class nsDOMXULCommandEvent : public nsDOMUIEvent,
                             public nsIDOMXULCommandEvent
{
public:
  nsDOMXULCommandEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext* aPresContext, nsInputEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMXULCommandEvent, nsDOMUIEvent)
  NS_DECL_NSIDOMXULCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

protected:
  
  nsInputEvent* Event() {
    return static_cast<nsInputEvent*>(mEvent);
  }

  nsCOMPtr<nsIDOMEvent> mSourceEvent;
};

#endif  
