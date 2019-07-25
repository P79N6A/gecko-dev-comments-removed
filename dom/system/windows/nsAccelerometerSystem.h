



































#ifndef nsAccelerometerSystem_h
#define nsAccelerometerSystem_h

#include "nsAccelerometer.h"
#include "nsAutoPtr.h"

class Sensor
{
 public:
  virtual PRBool Startup() = 0;
  virtual void Shutdown()  = 0;
  virtual void GetValues(double *x, double *y, double *z) = 0;
};

class nsAccelerometerSystem : public nsAccelerometer
{
 public:
  nsAccelerometerSystem();
  ~nsAccelerometerSystem();

  void Startup();
  void Shutdown();

  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);

  nsAutoPtr<Sensor> mSensor;
};

#endif
