





#ifndef MOZQORIENTATIONMODULE_H
#define MOZQORIENTATIONMODULE_H

#include <QtSensors/QOrientationReading>
#include <QtSensors/QOrientationFilter>
#include <QObject>
#include <QTransform>


class MozQOrientationSensorFilter : public QObject
                                  , public QtMobility::QOrientationFilter
{
    Q_OBJECT

public:
    MozQOrientationSensorFilter()
    {
        mWindowRotationAngle = 0;
    }

    virtual ~MozQOrientationSensorFilter(){}

    virtual bool filter(QtMobility::QOrientationReading* reading);

    static int GetWindowRotationAngle();
    static QTransform& GetRotationTransform();

Q_SIGNALS:
    void orientationChanged();

private:
    bool filter(QtMobility::QSensorReading *reading)
    {
        return filter(static_cast<QtMobility::QOrientationReading*>(reading));
    }

    static int mWindowRotationAngle;
    static QTransform mWindowRotationTransform;
};

#endif
