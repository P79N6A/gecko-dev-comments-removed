



































#ifndef nsDOMOrientationEvent_h__
#define nsDOMOrientationEvent_h__

#include "nsIDOMDeviceOrientationEvent.h"
#include "nsDOMEvent.h"

class nsDOMOrientationEvent : public nsDOMEvent,
                              public nsIDOMDeviceOrientationEvent
{
public:

  nsDOMOrientationEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent),
    mAlpha(0),
    mBeta(0),
    mGamma(0),
    mAbsolute(PR_TRUE) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMDEVICEORIENTATIONEVENT

protected:
  double mAlpha, mBeta, mGamma;
  PRBool mAbsolute;
};

#endif
