



































#ifndef nsDOMOrientationEvent_h__
#define nsDOMOrientationEvent_h__

#include "nsIDOMOrientationEvent.h"
#include "nsDOMEvent.h"

class nsDOMOrientationEvent : public nsDOMEvent,
                              public nsIDOMOrientationEvent
{
public:

  nsDOMOrientationEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent),
    mX(0),
    mY(0),
    mZ(0) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMORIENTATIONEVENT

protected:
  double mX, mY, mZ;
};

#endif
