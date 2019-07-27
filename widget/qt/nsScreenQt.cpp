



#include <QColor>
#include <QRect>
#include <QGuiApplication>
#include <QTransform>
#include <QScreen>

#include "nsScreenQt.h"
#include "nsXULAppAPI.h"

nsScreenQt::nsScreenQt(int aScreen)
    : mScreen(aScreen)
{
    
    
    
}

nsScreenQt::~nsScreenQt()
{
}

NS_IMETHODIMP
nsScreenQt::GetId(uint32_t* aId)
{
    *aId = mScreen;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetRect(int32_t *outLeft,int32_t *outTop,
                    int32_t *outWidth,int32_t *outHeight)
{
    QRect r = QGuiApplication::screens()[mScreen]->geometry();

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
    QRect r = QGuiApplication::screens()[mScreen]->geometry();

    *outTop = r.x();
    *outLeft = r.y();
    *outWidth = r.width();
    *outHeight = r.height();

    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetPixelDepth(int32_t *aPixelDepth)
{
    
    *aPixelDepth = QGuiApplication::primaryScreen()->depth();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenQt::GetColorDepth(int32_t *aColorDepth)
{
    
    return GetPixelDepth(aColorDepth);
}
