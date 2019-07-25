



































#ifndef nsDeviceSensors_h
#define nsDeviceSensors_h

#include "nsIDeviceSensors.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIDOMDeviceLightEvent.h"
#include "nsIDOMDeviceOrientationEvent.h"
#include "nsIDOMDeviceProximityEvent.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsDOMDeviceMotionEvent.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/HalSensor.h"
#include "nsDataHashtable.h"

#define NS_DEVICE_SENSORS_CID \
{ 0xecba5203, 0x77da, 0x465a, \
{ 0x86, 0x5e, 0x78, 0xb7, 0xaf, 0x10, 0xd8, 0xf7 } }

#define NS_DEVICE_SENSORS_CONTRACTID "@mozilla.org/devicesensors;1"

class nsIDOMWindow;

class nsDeviceSensors : public nsIDeviceSensors, public mozilla::hal::ISensorObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDEVICESENSORS

  nsDeviceSensors();

  virtual ~nsDeviceSensors();

  void Notify(const mozilla::hal::SensorData& aSensorData);

private:
  
  nsTArray<nsTArray<nsIDOMWindow*>* > mWindowListeners;

  void FireDOMLightEvent(nsIDOMEventTarget *target,
                         double value);

  void FireDOMProximityEvent(nsIDOMEventTarget *aTarget,
                             double aValue,
                             double aMin,
                             double aMax);

  void FireDOMOrientationEvent(class nsIDOMDocument *domDoc, 
                               class nsIDOMEventTarget *target,
                               double alpha,
                               double beta,
                               double gamma);

  void FireDOMMotionEvent(class nsIDOMDocument *domDoc, 
                          class nsIDOMEventTarget *target,
                          PRUint32 type,
                          double x,
                          double y,
                          double z);

  bool mEnabled;

  inline bool IsSensorEnabled(PRUint32 aType) {
    return mWindowListeners[aType]->Length() > 0;
  }

  mozilla::TimeStamp mLastDOMMotionEventTime;
  nsRefPtr<nsDOMDeviceAcceleration> mLastAcceleration;
  nsRefPtr<nsDOMDeviceAcceleration> mLastAccelerationIncluduingGravity;
  nsRefPtr<nsDOMDeviceRotationRate> mLastRotationRate;
};

#endif
