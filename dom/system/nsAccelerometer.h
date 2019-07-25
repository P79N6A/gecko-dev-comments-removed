



































#ifndef nsAccelerometer_h
#define nsAccelerometer_h

#include "nsIAccelerometer.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

#define NS_ACCELEROMETER_CID \
{ 0xecba5203, 0x77da, 0x465a, \
{ 0x86, 0x5e, 0x78, 0xb7, 0xaf, 0x10, 0xd8, 0xf7 } }

#define NS_ACCELEROMETER_CONTRACTID "@mozilla.org/accelerometer;1"

class nsIDOMWindow;

class nsAccelerometer : public nsIAccelerometerUpdate
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCELEROMETER
  NS_DECL_NSIACCELEROMETERUPDATE

  nsAccelerometer();

  virtual ~nsAccelerometer();

  double mLastAlpha;
  double mLastBeta;
  double mLastGamma;

private:
  nsCOMArray<nsIAccelerationListener> mListeners;
  nsCOMArray<nsIDOMWindow> mWindowListeners;

  void StartDisconnectTimer();

  PRBool mStarted;
  PRBool mNewListener;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  static void TimeoutHandler(nsITimer *aTimer, void *aClosure);

 protected:

  PRUint32 mUpdateInterval;
  PRBool   mEnabled;

  virtual void Startup()  = 0;
  virtual void Shutdown() = 0;
};

#endif
