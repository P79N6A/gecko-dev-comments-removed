



































#ifndef nsDeviceMotionSystem_h
#define nsDeviceMotionSystem_h

#include "nsDeviceMotion.h"
#include "nsAutoPtr.h"

class Sensor
{
 public:
  virtual PRBool Startup() = 0;
  virtual void Shutdown()  = 0;
  virtual void GetValues(double *x, double *y, double *z) = 0;
};

class nsDeviceMotionSystem : public nsDeviceMotion
{
 public:
  nsDeviceMotionSystem();
  ~nsDeviceMotionSystem();

  void Startup();
  void Shutdown();

  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);

  nsAutoPtr<Sensor> mSensor;
};

#endif
