




































#include "mozilla/dom/ContentChild.h"
#include "nsAccelerometerSystem.h"

#include "AndroidBridge.h"
#include "nsXULAppAPI.h"

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
    if (XRE_GetProcessType() == GeckoProcessType_Default)
        AndroidBridge::Bridge()->EnableAccelerometer(true);
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendAddAccelerometerListener();
}

void nsAccelerometerSystem::Shutdown()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default)
        AndroidBridge::Bridge()->EnableAccelerometer(false);
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendRemoveAccelerometerListener();
}
