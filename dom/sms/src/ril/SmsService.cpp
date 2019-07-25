




































#include "mozilla/dom/sms/SmsMessage.h"
#include "SmsService.h"
#include "RadioManager.h"
#include "jsapi.h"

using mozilla::dom::telephony::RadioManager;

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsService, nsISmsService)

SmsService::SmsService()
  : mRIL(RadioManager::GetTelephone())
{
}

NS_IMETHODIMP
SmsService::HasSupport(bool* aHasSupport)
{
  *aHasSupport = true;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  if (!mRIL) {
    *aResult = 0;
    return NS_OK;
  }

  mRIL->GetNumberOfMessagesForText(aText, aResult);
  return NS_OK;
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber, const nsAString& aMessage)
{
  if (!mRIL) {
    return NS_OK;
  }

  mRIL->SendSMS(aNumber, aMessage);
  return NS_OK;
}

NS_IMETHODIMP
SmsService::CreateSmsMessage(PRInt32 aId,
                             const nsAString& aDelivery,
                             const nsAString& aSender,
                             const nsAString& aReceiver,
                             const nsAString& aBody,
                             const jsval& aTimestamp,
                             JSContext* aCx,
                             nsIDOMMozSmsMessage** aMessage)
{
  return SmsMessage::Create(
    aId, aDelivery, aSender, aReceiver, aBody, aTimestamp, aCx, aMessage);
}

} 
} 
} 
