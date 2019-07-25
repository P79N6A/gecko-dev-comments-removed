




#include "mozilla/dom/ContentChild.h"
#include "SmsIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "mozilla/dom/sms/SmsChild.h"
#include "mozilla/dom/sms/SmsMessage.h"
#include "SmsFilter.h"

namespace mozilla {
namespace dom {
namespace sms {

PSmsChild* SmsIPCService::sSmsChild = nullptr;

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
SmsIPCService::GetNumberOfMessagesForText(const nsAString& aText, uint16_t* aResult)
{
  GetSmsChild()->SendGetNumberOfMessagesForText(nsString(aText), aResult);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::Send(const nsAString& aNumber, const nsAString& aMessage,
                    int32_t aRequestId, uint64_t aProcessId)
{
  GetSmsChild()->SendSendMessage(nsString(aNumber), nsString(aMessage),
                                 aRequestId, ContentChild::GetSingleton()->GetID());

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::CreateSmsMessage(int32_t aId,
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




NS_IMETHODIMP
SmsIPCService::SaveReceivedMessage(const nsAString& aSender,
                                   const nsAString& aBody,
                                   uint64_t aDate, int32_t* aId)
{
  GetSmsChild()->SendSaveReceivedMessage(nsString(aSender), nsString(aBody),
                                         aDate, aId);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::SaveSentMessage(const nsAString& aReceiver,
                               const nsAString& aBody,
                               uint64_t aDate, int32_t* aId)
{
  GetSmsChild()->SendSaveSentMessage(nsString(aReceiver), nsString(aBody),
                                     aDate, aId);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetMessageMoz(int32_t aMessageId, int32_t aRequestId,
                             uint64_t aProcessId)
{
  GetSmsChild()->SendGetMessage(aMessageId, aRequestId,
                                ContentChild::GetSingleton()->GetID());
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::DeleteMessage(int32_t aMessageId, int32_t aRequestId,
                             uint64_t aProcessId)
{
  GetSmsChild()->SendDeleteMessage(aMessageId, aRequestId,
                                   ContentChild::GetSingleton()->GetID());
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::CreateMessageList(nsIDOMMozSmsFilter* aFilter, bool aReverse,
                                 int32_t aRequestId, uint64_t aProcessId)
{
  SmsFilter* filter = static_cast<SmsFilter*>(aFilter);
  GetSmsChild()->SendCreateMessageList(filter->GetData(), aReverse, aRequestId,
                                       ContentChild::GetSingleton()->GetID());

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetNextMessageInList(int32_t aListId, int32_t aRequestId,
                                    uint64_t aProcessId)
{
  GetSmsChild()->SendGetNextMessageInList(aListId, aRequestId,
                                          ContentChild::GetSingleton()->GetID());
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::ClearMessageList(int32_t aListId)
{
  GetSmsChild()->SendClearMessageList(aListId);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::MarkMessageRead(int32_t aMessageId, bool aValue,
                               int32_t aRequestId, uint64_t aProcessId)
{
  GetSmsChild()->SendMarkMessageRead(aMessageId, aValue, aRequestId,
                                     ContentChild::GetSingleton()->GetID());
  return NS_OK;
}

} 
} 
} 
