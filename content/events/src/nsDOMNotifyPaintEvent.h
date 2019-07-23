





































#ifndef nsDOMNotifyPaintEvent_h_
#define nsDOMNotifyPaintEvent_h_

#include "nsIDOMNotifyPaintEvent.h"
#include "nsDOMEvent.h"

class nsDOMNotifyPaintEvent : public nsIDOMNotifyPaintEvent,
                              public nsDOMEvent
{
public:
  nsDOMNotifyPaintEvent(nsPresContext* aPresContext,
                        nsNotifyPaintEvent* aEvent);
  virtual ~nsDOMNotifyPaintEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMNOTIFYPAINTEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

private:
  nsRegion GetRegion();
};

#endif 
