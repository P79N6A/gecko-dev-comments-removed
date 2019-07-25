



































#ifndef nsDeviceMotionSystem_h
#define nsDeviceMotionSystem_h

#include <QObject>
#include <QtSensors/QAccelerometer>
#include <QtSensors/QRotationSensor>
#include "nsDeviceMotion.h"

using namespace QtMobility;


class MozQAccelerometerSensorFilter;
class MozQRotationSensorFilter;

class nsDeviceMotionSystem : public nsDeviceMotion
{
public:
  nsDeviceMotionSystem();
  virtual ~nsDeviceMotionSystem();

private:
  virtual void Startup();
  virtual void Shutdown();

  QtMobility::QAccelerometer* mAccelerometer;
  MozQAccelerometerSensorFilter* mAccelerometerFilter;
  QtMobility::QRotationSensor* mRotation;
  MozQRotationSensorFilter* mRotationFilter;
};

class MozQAccelerometerSensorFilter : public QObject, public QAccelerometerFilter
{
    Q_OBJECT

public:
    MozQAccelerometerSensorFilter(nsDeviceMotionSystem& aSystem) : mSystem(aSystem) {}

    virtual ~MozQAccelerometerSensorFilter(){}

    virtual bool filter(QAccelerometerReading* reading);

private:
    bool filter(QSensorReading *reading) { return filter(static_cast<QAccelerometerReading*>(reading)); }
    nsDeviceMotionSystem& mSystem;
};

class MozQRotationSensorFilter : public QObject, public QRotationFilter
{
    Q_OBJECT

public:
    MozQRotationSensorFilter(nsDeviceMotionSystem& aSystem) : mSystem(aSystem) {}

    virtual ~MozQRotationSensorFilter(){}

    virtual bool filter(QRotationReading* reading);

private:
    bool filter(QSensorReading *reading) { return filter(static_cast<QRotationReading*>(reading)); }
    nsDeviceMotionSystem& mSystem;
};

#endif 

