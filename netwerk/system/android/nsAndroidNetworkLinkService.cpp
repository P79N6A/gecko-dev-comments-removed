





































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
nsAndroidNetworkLinkService::GetIsLinkUp(PRBool *aIsUp)
{
  NS_ENSURE_TRUE(mozilla::AndroidBridge::Bridge(), NS_ERROR_NOT_IMPLEMENTED);

  *aIsUp = mozilla::AndroidBridge::Bridge()->IsNetworkLinkUp();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkStatusKnown(PRBool *aIsKnown)
{
  NS_ENSURE_TRUE(mozilla::AndroidBridge::Bridge(), NS_ERROR_NOT_IMPLEMENTED);

  *aIsKnown = mozilla::AndroidBridge::Bridge()->IsNetworkLinkKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidNetworkLinkService::GetLinkType(PRUint32 *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);
  NS_ENSURE_TRUE(mozilla::AndroidBridge::Bridge(), NS_ERROR_UNEXPECTED);

  *aLinkType = mozilla::AndroidBridge::Bridge()->GetNetworkLinkType();
  return NS_OK;
}
