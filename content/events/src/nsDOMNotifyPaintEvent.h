





































#ifndef nsDOMNotifyPaintEvent_h_
#define nsDOMNotifyPaintEvent_h_

#include "nsIDOMNotifyPaintEvent.h"
#include "nsDOMEvent.h"

class nsDOMNotifyPaintEvent : public nsIDOMNotifyPaintEvent,
                              public nsDOMEvent
{
public:
  nsDOMNotifyPaintEvent(nsPresContext*  aPresContext,
                        nsEvent*        aEvent,
                        PRUint32        aEventType,
                        const nsRegion* aSameOriginRegion,
                        const nsRegion* aCrossDocRegion);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMNOTIFYPAINTEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

private:
  nsRegion GetRegion();

  nsRegion mSameDocRegion;
  nsRegion mCrossDocRegion;
};

#endif 
