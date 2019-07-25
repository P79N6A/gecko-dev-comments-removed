




































#include "nsAccelerometerSystem.h"

#include "AndroidBridge.h"

using namespace mozilla;

extern nsAccelerometerSystem *gAccel;

nsAccelerometerSystem::nsAccelerometerSystem()
{
    gAccel = this;
}

nsAccelerometerSystem::~nsAccelerometerSystem()
{
}

void nsAccelerometerSystem::Startup()
{
    if (AndroidBridge::Bridge())
        AndroidBridge::Bridge()->EnableAccelerometer(true);
}

void nsAccelerometerSystem::Shutdown()
{
    if (AndroidBridge::Bridge())
        AndroidBridge::Bridge()->EnableAccelerometer(false);
}
