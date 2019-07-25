



#include <qcolor.h>
#include <qcolormap.h>
#include <qrect.h>
#include <qdesktopwidget.h>
#include <qapplication.h>
#include <QTransform>

#include "nsScreenQt.h"
#include "nsXULAppAPI.h"

#ifdef MOZ_ENABLE_QTMOBILITY
#include "mozqorientationsensorfilter.h"
#endif

#ifdef MOZ_ENABLE_QMSYSTEM2
#include <qmdisplaystate.h>
using namespace mozilla;

const int DISPLAY_BLANK_TIMEOUT = 10800; 
const int DISPLAY_DIM_TIMEOUT = 10620; 

#endif

nsScreenQt::nsScreenQt(int aScreen)
    : mScreen(aScreen)
#ifdef MOZ_ENABLE_QMSYSTEM2
    , mDisplayState(nullptr)
#endif
{
    
    
    
}

nsScreenQt::~nsScreenQt()
{
#ifdef MOZ_ENABLE_QMSYSTEM2
    delete mDisplayState;
    mDisplayState = nullptr;
#endif
}

NS_IMETHODIMP
nsScreenQt::GetRect(int32_t *outLeft,int32_t *outTop,
                    int32_t *outWidth,int32_t *outHeight)
{
    QRect r = QApplication::desktop()->screenGeometry(mScreen);
#ifdef MOZ_ENABLE_QTMOBILITY
    r = MozQOrientationSensorFilter::GetRotationTransform().mapRect(r);
    
    
    r.moveTo(0, 0);
#endif

    *outTop = r.x();
    *outLeft = r.y();
    *outWidth = r.width();
    *outHeight = r.height();

    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetAvailRect(int32_t *outLeft,int32_t *outTop,
                         int32_t *outWidth,int32_t *outHeight)
{
    QRect r = QApplication::desktop()->screenGeometry(mScreen);
#ifdef MOZ_ENABLE_QTMOBILITY
    r = MozQOrientationSensorFilter::GetRotationTransform().mapRect(r);
#endif

    *outTop = r.x();
    *outLeft = r.y();
    *outWidth = r.width();
    *outHeight = r.height();

    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetPixelDepth(int32_t *aPixelDepth)
{
    
    *aPixelDepth = (int32_t)QColormap::instance().depth();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetColorDepth(int32_t *aColorDepth)
{
    
    return GetPixelDepth(aColorDepth);
}

#ifdef MOZ_ENABLE_QMSYSTEM2
void
nsScreenQt::ApplyMinimumBrightness(uint32_t aType)
{
    
    
    
    
    delete mDisplayState;
    mDisplayState = nullptr;

    if( aType == BRIGHTNESS_FULL) {
        mDisplayState = new MeeGo::QmDisplayState();

        
        
        mDisplayState->setDisplayBlankTimeout( DISPLAY_BLANK_TIMEOUT  );
        mDisplayState->setDisplayDimTimeout( DISPLAY_DIM_TIMEOUT  );
        mDisplayState->setDisplayBrightnessValue( mDisplayState->getMaxDisplayBrightnessValue() );
        mDisplayState->set(MeeGo::QmDisplayState::On);
     }
}
#endif
