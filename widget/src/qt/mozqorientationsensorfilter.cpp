




































#include "mozqorientationsensorfilter.h"
#ifdef MOZ_ENABLE_MEEGOTOUCH
#include <MApplication>
#include <MWindow>
#endif
#include "nsXULAppAPI.h"

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

#ifdef MOZ_ENABLE_MEEGOTOUCH
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        MWindow* window = MApplication::activeWindow();
        if (window) {
            window->setOrientationAngle((M::OrientationAngle)mWindowRotationAngle);
        }
    }
#else
    emit orientationChanged();
#endif

    return true; 
}

int
MozQOrientationSensorFilter::GetWindowRotationAngle()
{
#ifdef MOZ_ENABLE_MEEGOTOUCH
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        MWindow* window = MApplication::activeWindow();
        if (window) {
            M::OrientationAngle angle = window->orientationAngle();
            if (mWindowRotationAngle != angle) {
                mWindowRotationAngle = angle;
                mWindowRotationTransform = QTransform();
                mWindowRotationTransform.rotate(mWindowRotationAngle);
            }
        }
    }
#endif
    return mWindowRotationAngle;
}

QTransform&
MozQOrientationSensorFilter::GetRotationTransform()
{
    GetWindowRotationAngle();
    return mWindowRotationTransform;
}

