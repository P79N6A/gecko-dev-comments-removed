





































#include "nsScreenManagerAndroid.h"
#include "nsWindow.h"
#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS2(nsScreenAndroid, nsIScreen, nsIScreen_MOZILLA_2_0_BRANCH)

nsScreenAndroid::nsScreenAndroid(void *nativeScreen)
{
}

nsScreenAndroid::~nsScreenAndroid()
{
}

NS_IMETHODIMP
nsScreenAndroid::GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
    gfxIntSize sz = nsWindow::GetAndroidBounds();

    *outLeft = 0;
    *outTop = 0;

    *outWidth = sz.width;
    *outHeight = sz.height;

    return NS_OK;
}


NS_IMETHODIMP
nsScreenAndroid::GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
    return GetRect(outLeft, outTop, outWidth, outHeight);
}



NS_IMETHODIMP
nsScreenAndroid::GetPixelDepth(PRInt32 *aPixelDepth)
{
    
    
    *aPixelDepth = 24;
    return NS_OK;
}


NS_IMETHODIMP
nsScreenAndroid::GetColorDepth(PRInt32 *aColorDepth)
{
    return GetPixelDepth(aColorDepth);
}

void
nsScreenAndroid::ApplyMinimumBrightness(PRUint32 aBrightness)
{
  AndroidBridge::Bridge()->SetKeepScreenOn(aBrightness == BRIGHTNESS_FULL);
}

NS_IMPL_ISUPPORTS1(nsScreenManagerAndroid, nsIScreenManager)

nsScreenManagerAndroid::nsScreenManagerAndroid()
{
    mOneScreen = new nsScreenAndroid(nsnull);
}

nsScreenManagerAndroid::~nsScreenManagerAndroid()
{
}

NS_IMETHODIMP
nsScreenManagerAndroid::GetPrimaryScreen(nsIScreen **outScreen)
{
    NS_IF_ADDREF(*outScreen = mOneScreen.get());
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerAndroid::ScreenForRect(PRInt32 inLeft,
                                      PRInt32 inTop,
                                      PRInt32 inWidth,
                                      PRInt32 inHeight,
                                      nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerAndroid::ScreenForNativeWidget(void *aWidget, nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerAndroid::GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
    *aNumberOfScreens = 1;
    return NS_OK;
}

