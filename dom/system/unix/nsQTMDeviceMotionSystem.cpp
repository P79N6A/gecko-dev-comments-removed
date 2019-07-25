




































#include "mozilla/dom/ContentChild.h"
#include "nsQTMDeviceMotionSystem.h"
#include <QObject>
#include <QtSensors/QAccelerometer>
#include <QtSensors/QRotationSensor>

#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace QtMobility;

bool
MozQAccelerometerSensorFilter::filter(QAccelerometerReading* reading)
{
    mSystem.DeviceMotionChanged(nsIDeviceMotionData::TYPE_ACCELERATION,
                                -reading->x(),
                                -reading->y(),
                                -reading->z());
    return true;
}

bool
MozQRotationSensorFilter::filter(QRotationReading* reading)
{
    
    
    
    
    
    mSystem.DeviceMotionChanged(nsIDeviceMotionData::TYPE_ORIENTATION,
                                reading->z(),
                                reading->x(),
                                reading->y());
    return true;
}

nsDeviceMotionSystem::nsDeviceMotionSystem()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        mAccelerometer = new QAccelerometer();
        mAccelerometerFilter = new MozQAccelerometerSensorFilter(*this);
        mAccelerometer->addFilter(mAccelerometerFilter);
        mRotation = new QRotationSensor();
        mRotationFilter = new MozQRotationSensorFilter(*this);
        mRotation->addFilter(mRotationFilter);
    }
}

nsDeviceMotionSystem::~nsDeviceMotionSystem()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        mAccelerometer->removeFilter(mAccelerometerFilter);
        mAccelerometer->stop();
        mRotation->removeFilter(mRotationFilter);
        mRotation->stop();
        delete mAccelerometer;
        delete mAccelerometerFilter;
        delete mRotation;
        delete mRotationFilter;
    }
}

void nsDeviceMotionSystem::Startup()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        mAccelerometer->start();
        if (!mAccelerometer->isActive()) {
            NS_WARNING("AccelerometerSensor didn't start!");
        }
        mRotation->start();
        if (!mRotation->isActive()) {
            NS_WARNING("RotationSensor didn't start!");
        }
    }
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendAddDeviceMotionListener();
}

void nsDeviceMotionSystem::Shutdown()
{
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        mAccelerometer->stop();
        mRotation->stop();
    }
    else
        mozilla::dom::ContentChild::GetSingleton()->
            SendRemoveDeviceMotionListener();
}
