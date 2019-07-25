



#include "qdesktopwidget.h"
#include "qapplication.h"

#include "nsScreenManagerQt.h"
#include "nsScreenQt.h"

nsScreenManagerQt::nsScreenManagerQt()
{
    desktop = 0;
    screens = 0;
}

nsScreenManagerQt::~nsScreenManagerQt()
{
    delete [] screens;
}


NS_IMPL_ISUPPORTS1(nsScreenManagerQt, nsIScreenManager)

void nsScreenManagerQt::init()
{
    if (desktop)
        return;

    desktop = QApplication::desktop();
    nScreens = desktop->numScreens();
    screens = new nsCOMPtr<nsIScreen>[nScreens];

    for (int i = 0; i < nScreens; ++i)
        screens[i] = new nsScreenQt(i);
}









NS_IMETHODIMP
nsScreenManagerQt::ScreenForRect(int32_t inLeft, int32_t inTop,
				 int32_t inWidth, int32_t inHeight,
				 nsIScreen **outScreen)
{
    if (!desktop)
        init();

    QRect r(inLeft, inTop, inWidth, inHeight);
    int best = 0;
    int area = 0;
    for (int i = 0; i < nScreens; ++i) {
        const QRect& rect = desktop->screenGeometry(i);
        QRect intersection = r&rect;
        int a = intersection.width()*intersection.height();
        if (a > area) {
            best = i;
            area = a;
        }
    }

    NS_IF_ADDREF(*outScreen = screens[best]);
    return NS_OK;
}







NS_IMETHODIMP
nsScreenManagerQt::GetPrimaryScreen(nsIScreen **aPrimaryScreen)
{
    if (!desktop)
        init();

    NS_IF_ADDREF(*aPrimaryScreen = screens[0]);
    return NS_OK;
}






NS_IMETHODIMP
nsScreenManagerQt::GetNumberOfScreens(uint32_t *aNumberOfScreens)
{
    if (!desktop)
        init();

    *aNumberOfScreens = desktop->numScreens();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerQt :: ScreenForNativeWidget (void *aWidget, nsIScreen **outScreen)
{
    
    
    QRect rect = static_cast<QWidget*>(aWidget)->geometry();
    return ScreenForRect(rect.x(), rect.y(), rect.width(), rect.height(), outScreen);
}
