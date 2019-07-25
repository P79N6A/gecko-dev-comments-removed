



































#ifndef nsAccelerometerAndroid_h
#define nsAccelerometerAndroid_h

#include "nsAccelerometer.h"

class nsAccelerometerAndroid : public nsAccelerometer
{
public:
  nsAccelerometerAndroid();
  virtual ~nsAccelerometerAndroid();

private:
  virtual void Startup();
  virtual void Shutdown();
};

#endif 

