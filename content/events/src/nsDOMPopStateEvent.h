

































#ifndef nsDOMPopStateEvent_h__
#define nsDOMPopStateEvent_h__

class nsEvent;

#include "nsIDOMPopStateEvent.h"
#include "nsDOMEvent.h"
#include "nsIVariant.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMPopStateEvent : public nsDOMEvent,
                           public nsIDOMPopStateEvent
{
public:
  nsDOMPopStateEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)  
  {
  }

  virtual ~nsDOMPopStateEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMPopStateEvent, nsDOMEvent)

  NS_DECL_NSIDOMPOPSTATEEVENT

  NS_FORWARD_TO_NSDOMEVENT

protected:
  nsCOMPtr<nsIVariant> mState;
};

nsresult NS_NewDOMPopStateEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsEvent* aEvent);

#endif 
