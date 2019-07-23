





































#ifndef nsDOMBeforeUnloadEvent_h__
#define nsDOMBeforeUnloadEvent_h__

#include "nsIDOMBeforeUnloadEvent.h"
#include "nsDOMEvent.h"

class nsDOMBeforeUnloadEvent : public nsIDOMBeforeUnloadEvent,
                               public nsDOMEvent
{
public:
  nsDOMBeforeUnloadEvent(nsPresContext* aPresContext,
                         nsBeforePageUnloadEvent* aEvent);
  virtual ~nsDOMBeforeUnloadEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMBEFOREUNLOADEVENT
};

#endif
