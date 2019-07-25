



































#ifndef nsDeviceMotion_h
#define nsDeviceMotion_h

#include "nsIDeviceMotion.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

#define NS_DEVICE_MOTION_CID \
{ 0xecba5203, 0x77da, 0x465a, \
{ 0x86, 0x5e, 0x78, 0xb7, 0xaf, 0x10, 0xd8, 0xf7 } }

#define NS_DEVICE_MOTION_CONTRACTID "@mozilla.org/devicemotion;1"

class nsIDOMWindow;

class nsDeviceMotion : public nsIDeviceMotionUpdate
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDEVICEMOTION
  NS_DECL_NSIDEVICEMOTIONUPDATE

  nsDeviceMotion();

  virtual ~nsDeviceMotion();

private:
  nsCOMArray<nsIDeviceMotionListener> mListeners;
  nsTArray<nsIDOMWindow*> mWindowListeners;

  void StartDisconnectTimer();

  bool mStarted;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  static void TimeoutHandler(nsITimer *aTimer, void *aClosure);

 protected:

  void FireNeedsCalibration(nsIDOMDocument *domdoc,
			    nsIDOMEventTarget *target);

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

  virtual void Startup()  = 0;
  virtual void Shutdown() = 0;
};

#endif
