





































#ifndef nsDOMPageTransitionEvent_h__
#define nsDOMPageTransitionEvent_h__

#include "nsIDOMPageTransitionEvent.h"
#include "nsDOMEvent.h"

class nsDOMPageTransitionEvent : public nsDOMEvent,
                                 public nsIDOMPageTransitionEvent
{
public:
  nsDOMPageTransitionEvent(nsPresContext* aPresContext, nsEvent* aEvent) :
  nsDOMEvent(aPresContext, aEvent), mPersisted(false) {}

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMPAGETRANSITIONEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual const nsIID& EventInitIID() { return NS_GET_IID(nsIPageTransitionEventInit); }
  virtual nsresult InitFromCtor(const nsAString& aType, nsISupports* aDict,
                                JSContext* aCx, JSObject* aObj);
protected:
  bool mPersisted;
};

#endif 
