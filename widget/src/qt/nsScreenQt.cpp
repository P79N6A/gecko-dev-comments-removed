






































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
    , mDisplayState(nsnull)
#endif
{
    
    
    
}

nsScreenQt::~nsScreenQt()
{
#ifdef MOZ_ENABLE_QMSYSTEM2
    delete mDisplayState;
    mDisplayState = nsnull;
#endif
}


#ifdef MOZ_ENABLE_QMSYSTEM2
NS_IMPL_ISUPPORTS2(nsScreenQt, nsIScreen, nsIScreen_MOZILLA_2_0_BRANCH)
#else
NS_IMPL_ISUPPORTS1(nsScreenQt, nsIScreen)
#endif

NS_IMETHODIMP
nsScreenQt::GetRect(PRInt32 *outLeft,PRInt32 *outTop,
                    PRInt32 *outWidth,PRInt32 *outHeight)
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
nsScreenQt::GetAvailRect(PRInt32 *outLeft,PRInt32 *outTop,
                         PRInt32 *outWidth,PRInt32 *outHeight)
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
nsScreenQt::GetPixelDepth(PRInt32 *aPixelDepth)
{
    
    *aPixelDepth = (PRInt32)QColormap::instance().depth();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetColorDepth(PRInt32 *aColorDepth)
{
    
    return GetPixelDepth(aColorDepth);
}

#ifdef MOZ_ENABLE_QMSYSTEM2
void
nsScreenQt::ApplyMinimumBrightness(PRUint32 aType)
{
    
    
    
    
    delete mDisplayState;
    mDisplayState = nsnull;

    if( aType == BRIGHTNESS_FULL) {
        mDisplayState = new MeeGo::QmDisplayState();

        
        
        mDisplayState->setDisplayBlankTimeout( DISPLAY_BLANK_TIMEOUT  );
        mDisplayState->setDisplayDimTimeout( DISPLAY_DIM_TIMEOUT  );
        mDisplayState->setDisplayBrightnessValue( mDisplayState->getMaxDisplayBrightnessValue() );
        mDisplayState->set(MeeGo::QmDisplayState::On);
     }
}
#endif
