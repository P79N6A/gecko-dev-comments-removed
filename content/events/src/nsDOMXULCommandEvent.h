







































#ifndef nsDOMXULCommandEvent_h_
#define nsDOMXULCommandEvent_h_

#include "nsDOMUIEvent.h"
#include "nsIDOMXULCommandEvent.h"

class nsDOMXULCommandEvent : public nsDOMUIEvent,
                             public nsIDOMXULCommandEvent
{
public:
  nsDOMXULCommandEvent(nsPresContext* aPresContext, nsXULCommandEvent* aEvent);
  virtual ~nsDOMXULCommandEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMXULCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

private:
  
  nsXULCommandEvent* Event() {
    return NS_STATIC_CAST(nsXULCommandEvent*, mEvent);
  }
};

#endif  
