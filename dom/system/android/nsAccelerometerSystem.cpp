



































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
    AndroidBridge::Bridge()->EnableAccelerometer(true);
}

void nsAccelerometerSystem::Shutdown()
{
    AndroidBridge::Bridge()->EnableAccelerometer(false);
}
