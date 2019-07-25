



































#ifndef nsDeviceMotionSystem_h
#define nsDeviceMotionSystem_h

#include <IOKit/IOKitLib.h>
#include <mach/mach_port.h>

#include "nsDeviceMotion.h"

class nsDeviceMotionSystem : public nsDeviceMotion
{
 public:
  nsDeviceMotionSystem();
  ~nsDeviceMotionSystem();

  void Startup();
  void Shutdown();

  io_connect_t mSmsConnection;
  nsCOMPtr<nsITimer> mUpdateTimer;
  static void UpdateHandler(nsITimer *aTimer, void *aClosure);
};

#endif
