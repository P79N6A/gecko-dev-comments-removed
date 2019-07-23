



































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

private:
  nsCOMArray<nsIAccelerationListener> mListeners;
  nsCOMArray<nsIDOMWindow> mWindowListeners;

  void startDisconnectTimer();

  PRBool mStarted;
  nsCOMPtr<nsITimer> mTimer;
  static void TimeoutHandler(nsITimer *aTimer, void *aClosure);

protected:

  virtual void Startup()  = 0;
  virtual void Shutdown() = 0;
};

#endif
