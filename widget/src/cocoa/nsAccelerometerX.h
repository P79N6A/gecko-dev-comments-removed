



































#ifndef nsAccelerometerX_h
#define nsAccelerometerX_h

#include <IOKit/IOKitLib.h>
#include <mach/mach_port.h>

#include "nsAccelerometer.h"

class nsAccelerometerX : public nsAccelerometer
{
 public:
  nsAccelerometerX();
  ~nsAccelerometerX();

  void Startup();
  void Shutdown();

  io_connect_t mSmsConnection;
  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
