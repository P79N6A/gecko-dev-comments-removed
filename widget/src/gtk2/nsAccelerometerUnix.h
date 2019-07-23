




































#ifndef nsAccelerometerUnix_h
#define nsAccelerometerUnix_h

#include <unistd.h>
#include "nsAccelerometer.h"

enum nsAccelerometerUnixDriver
{
  eNoSensor,
  eAppleSensor,
  eIBMSensor,
  eMaemoSensor
};

class nsAccelerometerUnix : public nsAccelerometer
{
 public:
  nsAccelerometerUnix();
  ~nsAccelerometerUnix();

  void Startup();
  void Shutdown();

  FILE* mPositionFile;
  FILE* mCalibrateFile;
  nsAccelerometerUnixDriver mType;

  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
