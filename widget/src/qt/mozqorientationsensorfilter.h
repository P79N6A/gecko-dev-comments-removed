




































#ifndef MOZQORIENTATIONMODULE_H
#define MOZQORIENTATIONMODULE_H

#include <QtSensors/QOrientationReading>
#include <QtSensors/QOrientationFilter>
#include <QObject>
#include <QTransform>

using namespace QtMobility;

class MozQOrientationSensorFilter : public QObject, public QOrientationFilter
{
    Q_OBJECT

public:
    MozQOrientationSensorFilter()
    {
        mWindowRotationAngle = 0;
    }

    virtual ~MozQOrientationSensorFilter(){}

    virtual bool filter(QOrientationReading* reading);

    static int GetWindowRotationAngle();
    static QTransform& GetRotationTransform();

signals:
    void orientationChanged();

private:
    bool filter(QSensorReading *reading) { return filter(static_cast<QOrientationReading*>(reading)); }

    static int mWindowRotationAngle;
    static QTransform mWindowRotationTransform;
};

#endif
