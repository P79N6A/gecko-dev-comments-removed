




#include "mozilla/dom/telephony/TelephonyFactory.h"
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
#include "nsIGonkTelephonyProvider.h"
#endif
#include "nsServiceManagerUtils.h"
#include "nsXULAppAPI.h"
#include "ipc/TelephonyIPCProvider.h"

USING_TELEPHONY_NAMESPACE

 already_AddRefed<nsITelephonyProvider>
TelephonyFactory::CreateTelephonyProvider()
{
  nsCOMPtr<nsITelephonyProvider> provider;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    provider = new TelephonyIPCProvider();
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
  } else {
    provider = do_CreateInstance(GONK_TELEPHONY_PROVIDER_CONTRACTID);
#endif
  }

  return provider.forget();
}
