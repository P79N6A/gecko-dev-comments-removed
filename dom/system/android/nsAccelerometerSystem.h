



































#ifndef nsAccelerometerSystem_h
#define nsAccelerometerSystem_h

#include "nsAccelerometer.h"

class nsAccelerometerSystem : public nsAccelerometer
{
public:
  nsAccelerometerSystem();
  virtual ~nsAccelerometerSystem();

private:
  virtual void Startup();
  virtual void Shutdown();
};

#endif 

