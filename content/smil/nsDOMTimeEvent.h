




































#ifndef NS_DOMTIMEEVENT_H_
#define NS_DOMTIMEEVENT_H_

#include "nsIDOMTimeEvent.h"
#include "nsDOMEvent.h"

class nsDOMTimeEvent : public nsDOMEvent,
                       public nsIDOMTimeEvent
{
public:
  nsDOMTimeEvent(nsPresContext* aPresContext, nsEvent* aEvent);
                     
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMTimeEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMTIMEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

private:
  nsCOMPtr<nsIDOMAbstractView> mView;
  PRInt32                      mDetail;
};

#endif 
