




#include "SmsServicesFactory.h"
#include "nsXULAppAPI.h"
#include "ipc/SmsIPCService.h"
#ifdef MOZ_WIDGET_ANDROID
#include "android/MobileMessageDatabaseService.h"
#include "android/SmsService.h"
#elif defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
#include "gonk/SmsService.h"
#endif
#include "nsServiceManagerUtils.h"

#define RIL_MMSSERVICE_CONTRACTID "@mozilla.org/mms/rilmmsservice;1"
#define RIL_MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1"

namespace mozilla {
namespace dom {
namespace mobilemessage {

 already_AddRefed<nsISmsService>
SmsServicesFactory::CreateSmsService()
{
  nsCOMPtr<nsISmsService> smsService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsService = new SmsIPCService();
  } else {
#ifdef MOZ_WIDGET_ANDROID
    smsService = new SmsService();
#elif defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
    smsService = new SmsService();
#endif
  }

  return smsService.forget();
}

 already_AddRefed<nsIMobileMessageDatabaseService>
SmsServicesFactory::CreateMobileMessageDatabaseService()
{
  nsCOMPtr<nsIMobileMessageDatabaseService> mobileMessageDBService;
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mobileMessageDBService = new SmsIPCService();
  } else {
#ifdef MOZ_WIDGET_ANDROID
    mobileMessageDBService = new MobileMessageDatabaseService();
#elif defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
    mobileMessageDBService = do_GetService(RIL_MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
#endif
  }

  return mobileMessageDBService.forget();
}

 already_AddRefed<nsIMmsService>
SmsServicesFactory::CreateMmsService()
{
  nsCOMPtr<nsIMmsService> mmsService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mmsService = new SmsIPCService();
  } else {
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
    mmsService = do_CreateInstance(RIL_MMSSERVICE_CONTRACTID);
#endif
  }

  return mmsService.forget();
}

} 
} 
} 
