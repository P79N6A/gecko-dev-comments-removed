




































#ifndef nsDOMCommandEvent_h__
#define nsDOMCommandEvent_h__

#include "nsIDOMCommandEvent.h"
#include "nsDOMEvent.h"

class nsDOMCommandEvent : public nsDOMEvent,
                          public nsIDOMCommandEvent
{
public:
  nsDOMCommandEvent(nsPresContext* aPresContext,
                    nsCommandEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
};

#endif
