




































#ifndef nsDOMMouseScrollEvent_h__
#define nsDOMMouseScrollEvent_h__

#include "nsIDOMMouseScrollEvent.h"
#include "nsDOMMouseEvent.h"

class nsDOMMouseScrollEvent : public nsDOMMouseEvent,
                              public nsIDOMMouseScrollEvent
{
public:
  nsDOMMouseScrollEvent(nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMMouseScrollEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMMOUSESCROLLEVENT
  
  
  NS_FORWARD_TO_NSDOMMOUSEEVENT
};

#endif
