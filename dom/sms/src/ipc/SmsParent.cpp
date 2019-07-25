




































#include "SmsParent.h"
#include "nsISmsService.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "Constants.h"
#include "nsIDOMSmsMessage.h"
#include "mozilla/unused.h"
#include "SmsMessage.h"
#include "nsISmsDatabaseService.h"

namespace mozilla {
namespace dom {
namespace sms {

nsTArray<SmsParent*>* SmsParent::gSmsParents = nsnull;

NS_IMPL_ISUPPORTS1(SmsParent, nsIObserver)

 void
SmsParent::GetAll(nsTArray<SmsParent*>& aArray)
{
  if (!gSmsParents) {
    aArray.Clear();
    return;
  }

  aArray = *gSmsParents;
}

SmsParent::SmsParent()
{
  if (!gSmsParents) {
    gSmsParents = new nsTArray<SmsParent*>();
  }

  gSmsParents->AppendElement(this);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  obs->AddObserver(this, kSmsReceivedObserverTopic, false);
  obs->AddObserver(this, kSmsSentObserverTopic, false);
  obs->AddObserver(this, kSmsDeliveredObserverTopic, false);
}

void
SmsParent::ActorDestroy(ActorDestroyReason why)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  obs->RemoveObserver(this, kSmsReceivedObserverTopic);
  obs->RemoveObserver(this, kSmsSentObserverTopic);
  obs->RemoveObserver(this, kSmsDeliveredObserverTopic);

  NS_ASSERTION(gSmsParents, "gSmsParents can't be null at that point!");
  gSmsParents->RemoveElement(this);
  if (gSmsParents->Length() == 0) {
    delete gSmsParents;
    gSmsParents = nsnull;
  }
}

NS_IMETHODIMP
SmsParent::Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData)
{
  if (!strcmp(aTopic, kSmsReceivedObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-received' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyReceivedMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsSentObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-sent' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifySentMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsDeliveredObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-delivered' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyDeliveredMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  return NS_OK;
}

bool
SmsParent::RecvHasSupport(bool* aHasSupport)
{
  *aHasSupport = false;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->HasSupport(aHasSupport);
  return true;
}

bool
SmsParent::RecvGetNumberOfMessagesForText(const nsString& aText, PRUint16* aResult)
{
  *aResult = 0;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->GetNumberOfMessagesForText(aText, aResult);
  return true;
}

bool
SmsParent::RecvSendMessage(const nsString& aNumber, const nsString& aMessage,
                           const PRInt32& aRequestId, const PRUint64& aProcessId)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->Send(aNumber, aMessage, aRequestId, aProcessId);
  return true;
}

bool
SmsParent::RecvSaveSentMessage(const nsString& aRecipient,
                               const nsString& aBody,
                               const PRUint64& aDate, PRInt32* aId)
{
  *aId = -1;

  nsCOMPtr<nsISmsDatabaseService> smsDBService =
    do_GetService(SMS_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsDBService, true);

  smsDBService->SaveSentMessage(aRecipient, aBody, aDate, aId);
  return true;
}

} 
} 
} 
