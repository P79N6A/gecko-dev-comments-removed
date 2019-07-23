





































#ifndef nsDOMNotifyPaintEvent_h_
#define nsDOMNotifyPaintEvent_h_

#include "nsIDOMNotifyPaintEvent.h"
#include "nsDOMEvent.h"
#include "nsPresContext.h"

class nsPaintRequestList;

class nsDOMNotifyPaintEvent : public nsIDOMNotifyPaintEvent,
                              public nsDOMEvent
{
public:
  nsDOMNotifyPaintEvent(nsPresContext*           aPresContext,
                        nsEvent*                 aEvent,
                        PRUint32                 aEventType,
                        nsInvalidateRequestList* aInvalidateRequests);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMNOTIFYPAINTEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

private:
  nsRegion GetRegion();

  nsTArray<nsInvalidateRequestList::Request> mInvalidateRequests;
};

#endif 
