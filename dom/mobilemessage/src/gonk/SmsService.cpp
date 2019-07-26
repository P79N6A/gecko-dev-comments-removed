




#include "SmsMessage.h"
#include "SmsService.h"
#include "jsapi.h"
#include "SmsSegmentInfo.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(SmsService, nsISmsService)

SmsService::SmsService()
{
  nsCOMPtr<nsIRadioInterfaceLayer> ril = do_GetService("@mozilla.org/ril;1");
  if (ril) {
    ril->GetRadioInterface(0, getter_AddRefs(mRadioInterface));
  }
  NS_WARN_IF_FALSE(mRadioInterface, "This shouldn't fail!");
}

NS_IMETHODIMP
SmsService::HasSupport(bool* aHasSupport)
{
  *aHasSupport = true;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetSegmentInfoForText(const nsAString & aText,
                                  nsIDOMMozSmsSegmentInfo** aResult)
{
  NS_ENSURE_TRUE(mRadioInterface, NS_ERROR_FAILURE);

  return mRadioInterface->GetSegmentInfoForText(aText, aResult);
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber,
                 const nsAString& aMessage,
                 nsIMobileMessageCallback* aRequest)
{
  NS_ENSURE_TRUE(mRadioInterface, NS_ERROR_FAILURE);

  return mRadioInterface->SendSMS(aNumber, aMessage, aRequest);
}

} 
} 
} 
