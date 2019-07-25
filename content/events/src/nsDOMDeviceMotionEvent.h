




































#ifndef nsDOMDeviceMotionEvent_h__
#define nsDOMDeviceMotionEvent_h__

#include "nsIDOMDeviceMotionEvent.h"
#include "nsDOMEvent.h"

class nsDOMDeviceRotationRate : public nsIDOMDeviceRotationRate
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDEVICEROTATIONRATE

  nsDOMDeviceRotationRate(double aAlpha, double aBeta, double aGamma);

private:
  ~nsDOMDeviceRotationRate();

protected:
  double mAlpha, mBeta, mGamma;
};

class nsDOMDeviceAcceleration : public nsIDOMDeviceAcceleration
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDEVICEACCELERATION

  nsDOMDeviceAcceleration(double aX, double aY, double aZ);

private:
  ~nsDOMDeviceAcceleration();

protected:
  double mX, mY, mZ;
};

class nsDOMDeviceMotionEvent : public nsDOMEvent,
                               public nsIDOMDeviceMotionEvent
{
public:

  nsDOMDeviceMotionEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent)
  {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMDEVICEMOTIONEVENT

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMDeviceMotionEvent, nsDOMEvent)

  nsCOMPtr<nsIDOMDeviceAcceleration> mAcceleration;
  nsCOMPtr<nsIDOMDeviceAcceleration> mAccelerationIncludingGravity;
  nsCOMPtr<nsIDOMDeviceRotationRate> mRotationRate;
  double mInterval;
};

#endif
