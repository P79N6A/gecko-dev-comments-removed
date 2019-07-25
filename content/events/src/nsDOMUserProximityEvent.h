



#ifndef nsDOMUserProximityEvent_h__
#define nsDOMUserProximityEvent_h__

#include "nsIDOMUserProximityEvent.h"
#include "nsDOMEvent.h"

class nsDOMUserProximityEvent
 : public nsDOMEvent
 , public nsIDOMUserProximityEvent
{
public:

  nsDOMUserProximityEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent),
    mNear(false) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMUSERPROXIMITYEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx,
                                jsval* aVal);
protected:
  bool mNear;
};

#endif
