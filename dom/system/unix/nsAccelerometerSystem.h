




































#ifndef nsAccelerometerSystem_h
#define nsAccelerometerSystem_h

#include <unistd.h>
#include "nsAccelerometer.h"

enum nsAccelerometerSystemDriver
{
  eNoSensor,
  eAppleSensor,
  eIBMSensor,
  eMaemoSensor,
  eHPdv7Sensor
};

class nsAccelerometerSystem : public nsAccelerometer
{
 public:
  nsAccelerometerSystem();
  ~nsAccelerometerSystem();

  void Startup();
  void Shutdown();

  FILE* mPositionFile;
  FILE* mCalibrateFile;
  nsAccelerometerSystemDriver mType;

  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
