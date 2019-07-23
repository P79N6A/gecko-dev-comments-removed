





































#ifndef nsDOMPopupBlockedEvent_h__
#define nsDOMPopupBlockedEvent_h__

#include "nsIDOMPopupBlockedEvent.h"
#include "nsDOMEvent.h"

class nsDOMPopupBlockedEvent : public nsIDOMPopupBlockedEvent,
                               public nsDOMEvent
{
public:

  nsDOMPopupBlockedEvent(nsPresContext* aPresContext, nsPopupBlockedEvent* aEvent);
  virtual ~nsDOMPopupBlockedEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMPOPUPBLOCKEDEVENT
};

#endif
