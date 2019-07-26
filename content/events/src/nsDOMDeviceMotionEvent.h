



#ifndef nsDOMDeviceMotionEvent_h__
#define nsDOMDeviceMotionEvent_h__

#include "nsIDOMDeviceMotionEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/Attributes.h"

class nsDOMDeviceRotationRate MOZ_FINAL : public nsIDOMDeviceRotationRate
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

class nsDOMDeviceAcceleration MOZ_FINAL : public nsIDOMDeviceAcceleration
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

class nsDOMDeviceMotionEvent MOZ_FINAL : public nsDOMEvent,
                                         public nsIDOMDeviceMotionEvent
{
public:

  nsDOMDeviceMotionEvent(mozilla::dom::EventTarget* aOwner,
                         nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent)
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
