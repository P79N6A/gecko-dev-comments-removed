

































#ifndef nsDOMPopStateEvent_h__
#define nsDOMPopStateEvent_h__

class nsEvent;

#include "nsIDOMPopStateEvent.h"
#include "nsIDOMPopStateEvent_MOZILLA_2_BRANCH.h"
#include "nsDOMEvent.h"
#include "nsIVariant.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMPopStateEvent : public nsDOMEvent,
                           public nsIDOMPopStateEvent,
                           public nsIDOMPopStateEvent_MOZILLA_2_BRANCH
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
  NS_DECL_NSIDOMPOPSTATEEVENT_MOZILLA_2_BRANCH

  NS_FORWARD_TO_NSDOMEVENT

protected:
  nsCOMPtr<nsIVariant> mState;
  PRBool mIsInitial;
};

nsresult NS_NewDOMPopStateEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsEvent* aEvent);

#endif 
