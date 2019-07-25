




































#include "mozqorientationsensorfilter.h"

int MozQOrientationSensorFilter::mWindowRotationAngle = 0;
QTransform MozQOrientationSensorFilter::mWindowRotationTransform;

bool
MozQOrientationSensorFilter::filter(QOrientationReading* reading)
{
    switch (reading->orientation()) {
    
    case QOrientationReading::TopDown:
        mWindowRotationAngle = 90;
        break;
    
    case QOrientationReading::TopUp:
        mWindowRotationAngle = 270;
        break;
    
    case QOrientationReading::LeftUp:
        mWindowRotationAngle = 180;
        break;
    
    case QOrientationReading::RightUp:
        mWindowRotationAngle = 0;
        break;
    
    case QOrientationReading::FaceUp:
    
    case QOrientationReading::FaceDown:
    
    case QOrientationReading::Undefined:
    default:
        return true;
    }

    mWindowRotationTransform = QTransform();
    mWindowRotationTransform.rotate(mWindowRotationAngle);

    emit orientationChanged();

    return true; 
}
