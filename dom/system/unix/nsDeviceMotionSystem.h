




































#ifndef nsDeviceMotionSystem_h
#define nsDeviceMotionSystem_h

#include <unistd.h>
#include "nsDeviceMotion.h"

enum nsAccelerometerSystemDriver
{
  eNoSensor,
  eAppleSensor,
  eIBMSensor,
  eMaemoSensor,
  eHPdv7Sensor
};

class nsDeviceMotionSystem : public nsDeviceMotion
{
 public:
  nsDeviceMotionSystem();
  ~nsDeviceMotionSystem();

  void Startup();
  void Shutdown();

  FILE* mPositionFile;
  FILE* mCalibrateFile;
  nsAccelerometerSystemDriver mType;

  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
