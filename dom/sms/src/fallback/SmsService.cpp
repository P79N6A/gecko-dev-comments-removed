




#include "mozilla/dom/sms/SmsMessage.h"
#include "SmsService.h"
#include "jsapi.h"

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
SmsService::GetNumberOfMessagesForText(const nsAString& aText, uint16_t* aResult)
{
  NS_ERROR("We should not be here!");
  *aResult = 0;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::Send(const nsAString& aNumber, const nsAString& aMessage,
                 int32_t aRequestId, uint64_t aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsService::CreateSmsMessage(int32_t aId,
                             const nsAString& aDelivery,
                             const nsAString& aSender,
                             const nsAString& aReceiver,
                             const nsAString& aBody,
                             const jsval& aTimestamp,
                             const bool aRead,
                             JSContext* aCx,
                             nsIDOMMozSmsMessage** aMessage)
{
  return SmsMessage::Create(aId, aDelivery, aSender, aReceiver, aBody,
                            aTimestamp, aRead, aCx, aMessage);
}

} 
} 
} 
