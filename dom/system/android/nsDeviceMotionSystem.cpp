




































#include "mozilla/dom/ContentChild.h"
#include "nsDeviceMotionSystem.h"

#include "AndroidBridge.h"
#include "nsXULAppAPI.h"

using namespace mozilla;

extern nsDeviceMotionSystem *gDeviceMotionSystem;

nsDeviceMotionSystem::nsDeviceMotionSystem()
{
    gDeviceMotionSystem = this;
}

nsDeviceMotionSystem::~nsDeviceMotionSystem()
{
}

void nsDeviceMotionSystem::Startup()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default)
        AndroidBridge::Bridge()->EnableDeviceMotion(true);
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendAddDeviceMotionListener();
}

void nsDeviceMotionSystem::Shutdown()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default)
        AndroidBridge::Bridge()->EnableDeviceMotion(false);
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendRemoveDeviceMotionListener();
}
