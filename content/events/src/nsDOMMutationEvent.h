





































#ifndef nsDOMMutationEvent_h__
#define nsDOMMutationEvent_h__

#include "nsCOMPtr.h"
#include "nsIDOMMutationEvent.h"
#include "nsDOMEvent.h"
#include "nsContentUtils.h"

class nsDOMMutationEvent : public nsDOMEvent,
                           public nsIDOMMutationEvent
{
public:
  nsDOMMutationEvent(nsPresContext* aPresContext, nsMutationEvent* aEvent);

  virtual ~nsDOMMutationEvent();
                     
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMMUTATIONEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
};

#endif
