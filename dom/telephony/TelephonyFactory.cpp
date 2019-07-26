




#include "mozilla/dom/telephony/TelephonyFactory.h"
#ifdef MOZ_WIDGET_GONK
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
#ifdef MOZ_WIDGET_GONK
  } else {
    provider = do_CreateInstance(GONK_TELEPHONY_PROVIDER_CONTRACTID);
#endif
  }

  return provider.forget();
}
