





#include "nsAndroidNetworkLinkService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"

#include "AndroidBridge.h"

using namespace mozilla::widget::android;

NS_IMPL_ISUPPORTS1(nsAndroidNetworkLinkService,
                   nsINetworkLinkService)

nsAndroidNetworkLinkService::nsAndroidNetworkLinkService()
{
}

nsAndroidNetworkLinkService::~nsAndroidNetworkLinkService()
{
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetIsLinkUp(bool *aIsUp)
{
  if (!mozilla::AndroidBridge::Bridge()) {
    
    NS_WARNING("GetIsLinkUp is not supported without a bridge connection");
    *aIsUp = true;
    return NS_OK;
  }

  *aIsUp = GeckoAppShell::IsNetworkLinkUp();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkStatusKnown(bool *aIsKnown)
{
  NS_ENSURE_TRUE(mozilla::AndroidBridge::Bridge(), NS_ERROR_NOT_IMPLEMENTED);

  *aIsKnown = GeckoAppShell::IsNetworkLinkKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkType(uint32_t *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);

  if (!mozilla::AndroidBridge::Bridge()) {
    
    NS_WARNING("GetLinkType is not supported without a bridge connection");
    *aLinkType = nsINetworkLinkService::LINK_TYPE_UNKNOWN;
    return NS_OK;
  }

  *aLinkType = GeckoAppShell::NetworkLinkType();
  return NS_OK;
}
