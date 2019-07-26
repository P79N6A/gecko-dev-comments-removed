




#include "SmsMessage.h"
#include "SmsSegmentInfo.h"
#include "SmsService.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(SmsService, nsISmsService)

NS_IMETHODIMP
SmsService::HasSupport(bool* aHasSupport)
{
  *aHasSupport = false;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetSegmentInfoForText(const nsAString & aText,
                                  nsIDOMMozSmsSegmentInfo** aResult)
{
  NS_ERROR("We should not be here!");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber,
                 const nsAString& aMessage,
                 nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
