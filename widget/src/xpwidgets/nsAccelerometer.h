



































#ifndef nsAccelerometer_h
#define nsAccelerometer_h

#include "nsIAccelerometer.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsIDOMWindow;

class nsAccelerometer : public nsIAccelerometer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCELEROMETER

  nsAccelerometer();

  virtual ~nsAccelerometer();

  
  void AccelerationChanged(double x, double y, double z);

  double mLastX;
  double mLastY;
  double mLastZ;

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

  virtual void Startup()  = 0;
  virtual void Shutdown() = 0;
};

#endif
