



































#ifndef nsAccelerometerSystem_h
#define nsAccelerometerSystem_h

#include <IOKit/IOKitLib.h>
#include <mach/mach_port.h>

#include "nsAccelerometer.h"

class nsAccelerometerSystem : public nsAccelerometer
{
 public:
  nsAccelerometerSystem();
  ~nsAccelerometerSystem();

  void Startup();
  void Shutdown();

  io_connect_t mSmsConnection;
  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
