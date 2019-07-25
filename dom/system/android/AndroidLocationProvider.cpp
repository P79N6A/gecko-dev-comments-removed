




































#include "nsGeolocation.h"
#include "nsGeoPosition.h"
#include "AndroidBridge.h"
#include "AndroidLocationProvider.h"

using namespace mozilla;

extern nsIGeolocationUpdate *gLocationCallback;

NS_IMPL_ISUPPORTS1(AndroidLocationProvider, nsIGeolocationProvider)

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
    if (!AndroidBridge::Bridge())
        return NS_ERROR_NOT_IMPLEMENTED;
    AndroidBridge::Bridge()->EnableLocation(true);
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
    if (!AndroidBridge::Bridge())
        return NS_ERROR_NOT_IMPLEMENTED;
    AndroidBridge::Bridge()->EnableLocation(false);
    return NS_OK;
}

