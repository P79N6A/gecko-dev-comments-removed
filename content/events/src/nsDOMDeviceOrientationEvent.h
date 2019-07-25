



































#ifndef nsDOMDeviceOrientationEvent_h__
#define nsDOMDeviceOrientationEvent_h__

#include "nsIDOMDeviceOrientationEvent.h"
#include "nsDOMEvent.h"

class nsDOMDeviceOrientationEvent : public nsDOMEvent,
                                    public nsIDOMDeviceOrientationEvent
{
public:

  nsDOMDeviceOrientationEvent(nsPresContext* aPresContext, nsEvent* aEvent)
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
  bool mAbsolute;
};

#endif
