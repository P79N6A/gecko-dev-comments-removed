






































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

nsScreenQt::nsScreenQt(int aScreen)
    : mScreen(aScreen)
{
    
    
    
}

nsScreenQt::~nsScreenQt()
{
    
}


NS_IMPL_ISUPPORTS1(nsScreenQt, nsIScreen)

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
