



































#ifndef nsDeviceMotionSystem_h
#define nsDeviceMotionSystem_h

#include "nsDeviceMotion.h"

class nsDeviceMotionSystem : public nsDeviceMotion
{
public:
  nsDeviceMotionSystem();
  virtual ~nsDeviceMotionSystem();

private:
  virtual void Startup();
  virtual void Shutdown();
};

#endif 

