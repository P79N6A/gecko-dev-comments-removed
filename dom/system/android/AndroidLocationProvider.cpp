




#include "nsGeolocation.h"
#include "nsGeoPosition.h"
#include "AndroidBridge.h"
#include "AndroidLocationProvider.h"

using namespace mozilla;

extern nsIGeolocationUpdate *gLocationCallback;

NS_IMPL_ISUPPORTS(AndroidLocationProvider, nsIGeolocationProvider)

AndroidLocationProvider::AndroidLocationProvider()
{
}

AndroidLocationProvider::~AndroidLocationProvider()
{
    NS_IF_RELEASE(gLocationCallback);
}

NS_IMETHODIMP
AndroidLocationProvider::Startup()
{
    mozilla::widget::android::GeckoAppShell::EnableLocation(true);
    return NS_OK;
}

NS_IMETHODIMP
AndroidLocationProvider::Watch(nsIGeolocationUpdate* aCallback)
{
    NS_IF_RELEASE(gLocationCallback);
    gLocationCallback = aCallback;
    NS_IF_ADDREF(gLocationCallback);
    return NS_OK;
}

NS_IMETHODIMP
AndroidLocationProvider::Shutdown()
{
    mozilla::widget::android::GeckoAppShell::EnableLocation(false);
    return NS_OK;
}

NS_IMETHODIMP
AndroidLocationProvider::SetHighAccuracy(bool enable)
{
    mozilla::widget::android::GeckoAppShell::EnableLocationHighAccuracy(enable);
    return NS_OK;
}
