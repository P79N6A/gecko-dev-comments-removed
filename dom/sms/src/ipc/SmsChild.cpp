




































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

} 
} 
} 
