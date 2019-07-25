

































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

  virtual const nsIID& EventInitIID() { return NS_GET_IID(nsIPopStateEventInit); }
  virtual nsresult InitFromCtor(const nsAString& aType, nsISupports* aDict,
                                JSContext* aCx, JSObject* aObj);
protected:
  nsCOMPtr<nsIVariant> mState;
};

#endif 
