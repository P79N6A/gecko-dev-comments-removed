




































#include "SmsService.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsService, nsISmsService)

NS_IMETHODIMP
SmsService::HasSupport(bool* aHasSupport)
{
  *aHasSupport = false;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  NS_ERROR("We should not be here!");
  *aResult = 0;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber, const nsAString& aMessage)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
