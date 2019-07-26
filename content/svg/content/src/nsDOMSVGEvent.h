




#ifndef __NS_SVGEVENT_H__
#define __NS_SVGEVENT_H__

#include "nsDOMEvent.h"
#include "nsIDOMSVGEvent.h"

class nsEvent;
class nsPresContext;

class nsDOMSVGEvent : public nsDOMEvent,
                      public nsIDOMSVGEvent
{
public:
  nsDOMSVGEvent(mozilla::dom::EventTarget* aOwner,
                nsPresContext* aPresContext, nsEvent* aEvent);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
};

#endif
