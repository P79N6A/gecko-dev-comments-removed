




#ifndef nsDOMMutationEvent_h__
#define nsDOMMutationEvent_h__

#include "nsIDOMMutationEvent.h"
#include "nsDOMEvent.h"

class nsDOMMutationEvent : public nsDOMEvent,
                           public nsIDOMMutationEvent
{
public:
  nsDOMMutationEvent(mozilla::dom::EventTarget* aOwner,
                     nsPresContext* aPresContext, nsMutationEvent* aEvent);

  virtual ~nsDOMMutationEvent();
                     
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMMUTATIONEVENT

  
  NS_FORWARD_TO_NSDOMEVENT
};

#endif
