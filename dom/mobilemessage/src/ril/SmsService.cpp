




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
  mRIL = do_GetService("@mozilla.org/ril;1");
  NS_WARN_IF_FALSE(mRIL, "This shouldn't fail!");
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
  NS_ENSURE_TRUE(mRIL, NS_ERROR_FAILURE);

  return mRIL->GetSegmentInfoForText(aText, aResult);
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber,
                 const nsAString& aMessage,
                 nsIMobileMessageCallback* aRequest)
{
  if (!mRIL) {
    return NS_OK;
  }
  mRIL->SendSMS(aNumber, aMessage, aRequest);
  return NS_OK;
}

} 
} 
} 
