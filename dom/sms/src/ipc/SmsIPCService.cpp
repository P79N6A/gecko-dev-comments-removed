




































#include "mozilla/dom/ContentChild.h"
#include "SmsIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "mozilla/dom/sms/SmsChild.h"
#include "mozilla/dom/sms/SmsMessage.h"

namespace mozilla {
namespace dom {
namespace sms {

PSmsChild* SmsIPCService::sSmsChild = nsnull;

NS_IMPL_ISUPPORTS2(SmsIPCService, nsISmsService, nsISmsDatabaseService)

 PSmsChild*
SmsIPCService::GetSmsChild()
{
  if (!sSmsChild) {
    sSmsChild = ContentChild::GetSingleton()->SendPSmsConstructor();
  }

  return sSmsChild;
}




NS_IMETHODIMP
SmsIPCService::HasSupport(bool* aHasSupport)
{
  GetSmsChild()->SendHasSupport(aHasSupport);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  GetSmsChild()->SendGetNumberOfMessagesForText(nsString(aText), aResult);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::Send(const nsAString& aNumber, const nsAString& aMessage,
                    PRInt32 aRequestId, PRUint64 aProcessId)
{
  GetSmsChild()->SendSendMessage(nsString(aNumber), nsString(aMessage),
                                 aRequestId, ContentChild::GetSingleton()->GetID());

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::CreateSmsMessage(PRInt32 aId,
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




NS_IMETHODIMP
SmsIPCService::SaveSentMessage(const nsAString& aReceiver,
                               const nsAString& aBody,
                               PRUint64 aDate, PRInt32* aId)
{
  GetSmsChild()->SendSaveSentMessage(nsString(aReceiver), nsString(aBody),
                                     aDate, aId);

  return NS_OK;
}

} 
} 
} 
