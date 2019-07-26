




#include "SmsChild.h"
#include "SmsMessage.h"
#include "Constants.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/dom/ContentChild.h"
#include "SmsRequestManager.h"
#include "SmsRequest.h"

using namespace mozilla;
using namespace mozilla::dom::sms;

namespace {

void
NotifyObserversWithSmsMessage(const char* aEventName,
                              const SmsMessageData& aMessageData)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  nsCOMPtr<SmsMessage> message = new SmsMessage(aMessageData);
  obs->NotifyObservers(message, aEventName, nullptr);
}

} 

namespace mozilla {
namespace dom {
namespace sms {

bool
SmsChild::RecvNotifyReceivedMessage(const SmsMessageData& aMessageData)
{
  NotifyObserversWithSmsMessage(kSmsReceivedObserverTopic, aMessageData);
  return true;
}

bool
SmsChild::RecvNotifySentMessage(const SmsMessageData& aMessageData)
{
  NotifyObserversWithSmsMessage(kSmsSentObserverTopic, aMessageData);
  return true;
}

bool
SmsChild::RecvNotifyDeliverySuccessMessage(const SmsMessageData& aMessageData)
{
  NotifyObserversWithSmsMessage(kSmsDeliverySuccessObserverTopic, aMessageData);
  return true;
}

bool
SmsChild::RecvNotifyDeliveryErrorMessage(const SmsMessageData& aMessageData)
{
  NotifyObserversWithSmsMessage(kSmsDeliveryErrorObserverTopic, aMessageData);
  return true;
}

bool
SmsChild::RecvNotifyRequestSmsSent(const SmsMessageData& aMessage,
                                   const int32_t& aRequestId,
                                   const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessage);
  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifySmsSent(aRequestId, message);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsSendFailed(const int32_t& aError,
                                         const int32_t& aRequestId,
                                         const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifySmsSendFailed(aRequestId, aError);

  return true;
}

bool
SmsChild::RecvNotifyRequestGotSms(const SmsMessageData& aMessage,
                                  const int32_t& aRequestId,
                                  const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessage);
  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyGotSms(aRequestId, message);

  return true;
}

bool
SmsChild::RecvNotifyRequestGetSmsFailed(const int32_t& aError,
                                        const int32_t& aRequestId,
                                        const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyGetSmsFailed(aRequestId, aError);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsDeleted(const bool& aDeleted,
                                      const int32_t& aRequestId,
                                      const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifySmsDeleted(aRequestId, aDeleted);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsDeleteFailed(const int32_t& aError,
                                           const int32_t& aRequestId,
                                           const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifySmsDeleteFailed(aRequestId, aError);

  return true;
}

bool
SmsChild::RecvNotifyRequestNoMessageInList(const int32_t& aRequestId,
                                           const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyNoMessageInList(aRequestId);
  return true;
}

bool
SmsChild::RecvNotifyRequestCreateMessageList(const int32_t& aListId,
                                             const SmsMessageData& aMessageData,
                                             const int32_t& aRequestId,
                                             const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessageData);
  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyCreateMessageList(aRequestId, aListId, message);
  return true;
}

bool
SmsChild::RecvNotifyRequestGotNextMessage(const SmsMessageData& aMessageData,
                                          const int32_t& aRequestId,
                                          const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessageData);
  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyGotNextMessage(aRequestId, message);
  return true;
}

bool
SmsChild::RecvNotifyRequestReadListFailed(const int32_t& aError,
                                          const int32_t& aRequestId,
                                          const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager = do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyReadMessageListFailed(aRequestId, aError);
  return true;
}

bool
SmsChild::RecvNotifyRequestMarkedMessageRead(const bool& aRead,
                                             const int32_t& aRequestId,
                                             const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager =
    do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyMarkedMessageRead(aRequestId, aRead);
  return true;
}

bool
SmsChild::RecvNotifyRequestMarkMessageReadFailed(const int32_t& aError,
                                                 const int32_t& aRequestId,
                                                 const uint64_t& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsISmsRequestManager> requestManager =
    do_GetService(SMS_REQUEST_MANAGER_CONTRACTID);
  requestManager->NotifyMarkMessageReadFailed(aRequestId, aError);

  return true;
}

} 
} 
} 
