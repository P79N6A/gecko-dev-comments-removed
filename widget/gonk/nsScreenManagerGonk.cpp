





































#include "nsScreenManagerGonk.h"
#include "nsWindow.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS2(nsScreenGonk, nsIScreen, nsIScreen)

nsScreenGonk::nsScreenGonk(void *nativeScreen)
{
}

nsScreenGonk::~nsScreenGonk()
{
}

NS_IMETHODIMP
nsScreenGonk::GetRect(PRInt32 *outLeft,  PRInt32 *outTop,
                      PRInt32 *outWidth, PRInt32 *outHeight)
{
    *outLeft = gScreenBounds.x;
    *outTop = gScreenBounds.y;

    *outWidth = gScreenBounds.width;
    *outHeight = gScreenBounds.height;

    return NS_OK;
}


NS_IMETHODIMP
nsScreenGonk::GetAvailRect(PRInt32 *outLeft,  PRInt32 *outTop,
                          PRInt32 *outWidth, PRInt32 *outHeight)
{
    return GetRect(outLeft, outTop, outWidth, outHeight);
}



NS_IMETHODIMP
nsScreenGonk::GetPixelDepth(PRInt32 *aPixelDepth)
{
    
    
    *aPixelDepth = 24;
    return NS_OK;
}


NS_IMETHODIMP
nsScreenGonk::GetColorDepth(PRInt32 *aColorDepth)
{
    return GetPixelDepth(aColorDepth);
}

NS_IMPL_ISUPPORTS1(nsScreenManagerGonk, nsIScreenManager)

nsScreenManagerGonk::nsScreenManagerGonk()
{
    mOneScreen = new nsScreenGonk(nsnull);
}

nsScreenManagerGonk::~nsScreenManagerGonk()
{
}

NS_IMETHODIMP
nsScreenManagerGonk::GetPrimaryScreen(nsIScreen **outScreen)
{
    NS_IF_ADDREF(*outScreen = mOneScreen.get());
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForRect(PRInt32 inLeft,
                                   PRInt32 inTop,
                                   PRInt32 inWidth,
                                   PRInt32 inHeight,
                                   nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForNativeWidget(void *aWidget, nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerGonk::GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
    *aNumberOfScreens = 1;
    return NS_OK;
}

