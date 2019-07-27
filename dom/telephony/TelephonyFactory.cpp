




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
  nsCOMPtr<nsITelephonyService> service;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    service = new TelephonyIPCService();
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
  } else {
    service = do_CreateInstance(GONK_TELEPHONY_SERVICE_CONTRACTID);
#endif
  }

  return service.forget();
}
