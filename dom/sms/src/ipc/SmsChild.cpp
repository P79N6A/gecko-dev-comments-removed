




































#include "SmsChild.h"
#include "SmsMessage.h"
#include "Constants.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/dom/ContentChild.h"
#include "SmsRequestManager.h"
#include "SmsRequest.h"

namespace mozilla {
namespace dom {
namespace sms {

bool
SmsChild::RecvNotifyReceivedMessage(const SmsMessageData& aMessageData)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return true;
  }

  nsCOMPtr<SmsMessage> message = new SmsMessage(aMessageData);
  obs->NotifyObservers(message, kSmsReceivedObserverTopic, nsnull);

  return true;
}

bool
SmsChild::RecvNotifySentMessage(const SmsMessageData& aMessageData)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return true;
  }

  nsCOMPtr<SmsMessage> message = new SmsMessage(aMessageData);
  obs->NotifyObservers(message, kSmsSentObserverTopic, nsnull);

  return true;
}

bool
SmsChild::RecvNotifyDeliveredMessage(const SmsMessageData& aMessageData)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return true;
  }

  nsCOMPtr<SmsMessage> message = new SmsMessage(aMessageData);
  obs->NotifyObservers(message, kSmsDeliveredObserverTopic, nsnull);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsSent(const SmsMessageData& aMessage,
                                   const PRInt32& aRequestId,
                                   const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessage);
  SmsRequestManager::GetInstance()->NotifySmsSent(aRequestId, message);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsSendFailed(const PRInt32& aError,
                                         const PRInt32& aRequestId,
                                         const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  SmsRequestManager::GetInstance()->NotifySmsSendFailed(aRequestId,
                                                        SmsRequest::ErrorType(aError));

  return true;
}

bool
SmsChild::RecvNotifyRequestGotSms(const SmsMessageData& aMessage,
                                  const PRInt32& aRequestId,
                                  const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessage);
  SmsRequestManager::GetInstance()->NotifyGotSms(aRequestId, message);

  return true;
}

bool
SmsChild::RecvNotifyRequestGetSmsFailed(const PRInt32& aError,
                                        const PRInt32& aRequestId,
                                        const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  SmsRequestManager::GetInstance()->NotifyGetSmsFailed(aRequestId,
                                                       SmsRequest::ErrorType(aError));

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsDeleted(const bool& aDeleted,
                                      const PRInt32& aRequestId,
                                      const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  SmsRequestManager::GetInstance()->NotifySmsDeleted(aRequestId, aDeleted);

  return true;
}

bool
SmsChild::RecvNotifyRequestSmsDeleteFailed(const PRInt32& aError,
                                           const PRInt32& aRequestId,
                                           const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  SmsRequestManager::GetInstance()->NotifySmsDeleteFailed(aRequestId,
                                                          SmsRequest::ErrorType(aError));

  return true;
}

bool
SmsChild::RecvNotifyRequestNoMessageInList(const PRInt32& aRequestId,
                                           const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  SmsRequestManager::GetInstance()->NotifyNoMessageInList(aRequestId);
  return true;
}

bool
SmsChild::RecvNotifyRequestCreateMessageList(const PRInt32& aListId,
                                             const SmsMessageData& aMessageData,
                                             const PRInt32& aRequestId,
                                             const PRUint64& aProcessId)
{
  if (ContentChild::GetSingleton()->GetID() != aProcessId) {
    return true;
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(aMessageData);
  SmsRequestManager::GetInstance()->NotifyCreateMessageList(aRequestId, aListId, message);
  return true;
}

} 
} 
} 
