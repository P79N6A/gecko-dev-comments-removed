





































#ifndef nsDOMPageTransitionEvent_h__
#define nsDOMPageTransitionEvent_h__

#include "nsIDOMPageTransitionEvent.h"
#include "nsDOMEvent.h"

class nsDOMPageTransitionEvent : public nsDOMEvent,
                                 public nsIDOMPageTransitionEvent
{
public:
  nsDOMPageTransitionEvent(nsPresContext* aPresContext, nsEvent* aEvent) :
  nsDOMEvent(aPresContext, aEvent), mPersisted(PR_FALSE) {}

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMPAGETRANSITIONEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
protected:
  PRBool mPersisted;
};

#endif 
