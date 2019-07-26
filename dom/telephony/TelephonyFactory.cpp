




#include "mozilla/dom/telephony/TelephonyFactory.h"
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
#include "nsIGonkTelephonyService.h"
#endif
#include "nsServiceManagerUtils.h"
#include "nsXULAppAPI.h"
#include "ipc/TelephonyIPCService.h"

USING_TELEPHONY_NAMESPACE

 already_AddRefed<nsITelephonyService>
TelephonyFactory::CreateTelephonyService()
{
  nsCOMPtr<nsITelephonyService> provider;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    provider = new TelephonyIPCService();
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
  } else {
    provider = do_CreateInstance(GONK_TELEPHONY_SERVICE_CONTRACTID);
#endif
  }

  return provider.forget();
}
