




































#ifndef __NS_SVGEVENT_H__
#define __NS_SVGEVENT_H__

#include "nsIDOMSVGEvent.h"
#include "nsDOMEvent.h"

class nsDOMSVGEvent : public nsDOMEvent,
                      public nsIDOMSVGEvent
{
public:
  nsDOMSVGEvent(nsPresContext* aPresContext, nsEvent* aEvent);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
};

#endif
