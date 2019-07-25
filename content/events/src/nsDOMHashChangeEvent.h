

































#ifndef nsDOMHashChangeEvent_h__
#define nsDOMHashChangeEvent_h__

class nsEvent;

#include "nsIDOMHashChangeEvent.h"
#include "nsDOMEvent.h"
#include "nsIVariant.h"
#include "nsCycleCollectionParticipant.h"

class nsDOMHashChangeEvent : public nsDOMEvent,
                             public nsIDOMHashChangeEvent
{
public:
  nsDOMHashChangeEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)
  {
  }

  virtual ~nsDOMHashChangeEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMHASHCHANGEEVENT

  NS_FORWARD_TO_NSDOMEVENT

  virtual const nsIID& EventInitIID() { return NS_GET_IID(nsIHashChangeEventInit); }
  virtual nsresult InitFromCtor(const nsAString& aType, nsISupports* aDict,
                                JSContext* aCx, JSObject* aObj);
protected:
  nsString mOldURL;
  nsString mNewURL;
};

#endif 
