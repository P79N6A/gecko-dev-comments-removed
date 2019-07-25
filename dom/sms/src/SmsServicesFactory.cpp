




































#include "SmsServicesFactory.h"
#include "nsXULAppAPI.h"
#include "SmsService.h"
#include "SmsIPCService.h"
#ifndef MOZ_B2G_RIL
#include "SmsDatabaseService.h"
#endif
#include "nsServiceManagerUtils.h"

#define RIL_SMS_DATABASE_SERVICE_CONTRACTID "@mozilla.org/sms/rilsmsdatabaseservice;1"

namespace mozilla {
namespace dom {
namespace sms {

 already_AddRefed<nsISmsService>
SmsServicesFactory::CreateSmsService()
{
  nsCOMPtr<nsISmsService> smsService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsService = new SmsIPCService();
  } else {
    smsService = new SmsService();
  }

  return smsService.forget();
}

 already_AddRefed<nsISmsDatabaseService>
SmsServicesFactory::CreateSmsDatabaseService()
{
  nsCOMPtr<nsISmsDatabaseService> smsDBService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsDBService = new SmsIPCService();
  } else {
#ifdef MOZ_B2G_RIL
    smsDBService = do_GetService(RIL_SMS_DATABASE_SERVICE_CONTRACTID);
#else
    smsDBService = new SmsDatabaseService();
#endif
  }

  return smsDBService.forget();
}

} 
} 
} 
