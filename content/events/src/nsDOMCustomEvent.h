




































#ifndef nsDOMCustomEvent_h__
#define nsDOMCustomEvent_h__

#include "nsIDOMCustomEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMCustomEvent : public nsDOMEvent,
                         public nsIDOMCustomEvent
{
public:
  nsDOMCustomEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)
  {
  }
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMCustomEvent, nsDOMEvent)

  NS_DECL_NSIDOMCUSTOMEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual const nsIID& EventInitIID() { return NS_GET_IID(nsICustomEventInit); }
  virtual nsresult InitFromCtor(const nsAString& aType, nsISupports* aDict,
                                JSContext* aCx, JSObject* aObj);
private:
  nsCOMPtr<nsIVariant> mDetail;
};

#endif 
