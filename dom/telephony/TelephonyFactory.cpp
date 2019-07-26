




#include "mozilla/dom/telephony/TelephonyFactory.h"
#include "nsServiceManagerUtils.h"
#include "nsXULAppAPI.h"
#include "TelephonyIPCProvider.h"

USING_TELEPHONY_NAMESPACE

 already_AddRefed<nsITelephonyProvider>
TelephonyFactory::CreateTelephonyProvider()
{
  nsCOMPtr<nsITelephonyProvider> provider;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    provider = new TelephonyIPCProvider();
  }

  return provider.forget();
}
