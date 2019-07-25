




































#include "SmsService.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsService, nsISmsService)

NS_IMETHODIMP
SmsService::HasSupport(bool* aHasSupport)
{
  *aHasSupport = true;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  if (!AndroidBridge::Bridge()) {
    *aResult = 0;
    return NS_OK;
  }

  *aResult = AndroidBridge::Bridge()->GetNumberOfMessagesForText(aText);
  return NS_OK;
}

} 
} 
} 
