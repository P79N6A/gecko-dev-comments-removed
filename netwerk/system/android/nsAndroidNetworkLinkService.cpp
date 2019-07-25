





































#include "nsAndroidNetworkLinkService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"

#include "AndroidBridge.h"

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

  *aIsUp = mozilla::AndroidBridge::Bridge()->IsNetworkLinkUp();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkStatusKnown(bool *aIsKnown)
{
  NS_ENSURE_TRUE(mozilla::AndroidBridge::Bridge(), NS_ERROR_NOT_IMPLEMENTED);

  *aIsKnown = mozilla::AndroidBridge::Bridge()->IsNetworkLinkKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkType(PRUint32 *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);

  
  *aLinkType = nsINetworkLinkService::LINK_TYPE_UNKNOWN;
  return NS_OK;
}
