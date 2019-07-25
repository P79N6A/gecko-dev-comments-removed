



#ifndef nsDOMDeviceLightEvent_h__
#define nsDOMDeviceLightEvent_h__

#include "nsIDOMDeviceLightEvent.h"
#include "nsDOMEvent.h"

class nsDOMDeviceLightEvent
 : public nsDOMEvent
 , public nsIDOMDeviceLightEvent
{
public:

  nsDOMDeviceLightEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent),
    mValue(0) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMDEVICELIGHTEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx,
                                jsval* aVal);
protected:
  double mValue;
};

#endif
