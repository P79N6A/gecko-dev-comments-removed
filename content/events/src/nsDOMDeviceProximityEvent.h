



#ifndef nsDOMDeviceProximityEvent_h__
#define nsDOMDeviceProximityEvent_h__

#include "nsIDOMDeviceProximityEvent.h"
#include "nsDOMEvent.h"

class nsDOMDeviceProximityEvent
 : public nsDOMEvent
 , public nsIDOMDeviceProximityEvent
{
public:

 nsDOMDeviceProximityEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent),
    mValue(-1),
    mMin(0),
    mMax(0) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMDEVICEPROXIMITYEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx,
                                jsval* aVal);
protected:
  double mValue, mMin, mMax;
};

#endif
