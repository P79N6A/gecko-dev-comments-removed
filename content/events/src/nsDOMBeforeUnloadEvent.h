





































#ifndef nsDOMBeforeUnloadEvent_h__
#define nsDOMBeforeUnloadEvent_h__

#include "nsIDOMBeforeUnloadEvent.h"
#include "nsDOMEvent.h"

class nsDOMBeforeUnloadEvent : public nsDOMEvent,
                               public nsIDOMBeforeUnloadEvent
{
public:
  nsDOMBeforeUnloadEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMBEFOREUNLOADEVENT
protected:
  nsString mText;
};

#endif 
