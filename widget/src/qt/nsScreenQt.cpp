






































#include "nsScreenQt.h"

#include <qcolor.h>
#include <qapplication.h>

#include "qtlog.h"

nsScreenQt::nsScreenQt(int aScreen)
{
    screen = aScreen;
    
    
    
}

nsScreenQt::~nsScreenQt()
{
    
}


NS_IMPL_ISUPPORTS1(nsScreenQt, nsIScreen)

    NS_IMETHODIMP
nsScreenQt::GetRect(PRInt32 *outLeft,PRInt32 *outTop,
                    PRInt32 *outWidth,PRInt32 *outHeight)
{
    QRect r = QApplication::desktop()->screenGeometry(screen);
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
    QRect r = QApplication::desktop()->availableGeometry(screen);
    *outTop = r.x();
    *outLeft = r.y();
    *outWidth = r.width();
    *outHeight = r.height();

    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetPixelDepth(PRInt32 *aPixelDepth)
{
    
    *aPixelDepth = (PRInt32)QColor::numBitPlanes();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetColorDepth(PRInt32 *aColorDepth)
{
    
    return GetPixelDepth(aColorDepth);
}
