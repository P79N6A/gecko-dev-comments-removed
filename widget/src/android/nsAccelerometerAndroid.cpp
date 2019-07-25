



































#include "nsAccelerometerAndroid.h"

#include "AndroidBridge.h"

using namespace mozilla;

extern nsAccelerometerAndroid *gAccel;

nsAccelerometerAndroid::nsAccelerometerAndroid()
{
    gAccel = this;
}

nsAccelerometerAndroid::~nsAccelerometerAndroid()
{
}

void nsAccelerometerAndroid::Startup()
{
    AndroidBridge::Bridge()->EnableAccelerometer(true);
}

void nsAccelerometerAndroid::Shutdown()
{
    AndroidBridge::Bridge()->EnableAccelerometer(false);
}
