




































#include "SmsManager.h"
#include "nsIDOMClassInfo.h"
#include "nsISmsService.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "Constants.h"
#include "SmsEvent.h"
#include "nsIDOMSmsMessage.h"





#define RECEIVED_EVENT_NAME NS_LITERAL_STRING("received")
#define SENT_EVENT_NAME     NS_LITERAL_STRING("sent")

DOMCI_DATA(MozSmsManager, mozilla::dom::sms::SmsManager)

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_CYCLE_COLLECTION_CLASS(SmsManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SmsManager,
                                                  nsDOMEventTargetWrapperCache)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(received)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(sent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SmsManager,
                                                nsDOMEventTargetWrapperCache)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(received)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(sent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SmsManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozSmsManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_ADDREF_INHERITED(SmsManager, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(SmsManager, nsDOMEventTargetWrapperCache)

void
SmsManager::Init(nsPIDOMWindow *aWindow, nsIScriptContext* aScriptContext)
{
  
  mOwner = aWindow;
  mScriptContext = aScriptContext;

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  
  if (!obs) {
    return;
  }

  obs->AddObserver(this, kSmsReceivedObserverTopic, false);
  obs->AddObserver(this, kSmsSentObserverTopic, false);
}

void
SmsManager::Shutdown()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  
  if (!obs) {
    return;
  }

  obs->RemoveObserver(this, kSmsReceivedObserverTopic);
  obs->RemoveObserver(this, kSmsSentObserverTopic);
}

NS_IMETHODIMP
SmsManager::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, NS_OK);

  smsService->GetNumberOfMessagesForText(aText, aResult);

  return NS_OK;
}

NS_IMETHODIMP
SmsManager::Send(const nsAString& aNumber, const nsAString& aMessage)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, NS_OK);

  smsService->Send(aNumber, aMessage);

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(SmsManager, received)
NS_IMPL_EVENT_HANDLER(SmsManager, sent)

nsresult
SmsManager::DispatchTrustedSmsEventToSelf(const nsAString& aEventName, nsIDOMMozSmsMessage* aMessage)
{
  nsRefPtr<nsDOMEvent> event = new SmsEvent(nsnull, nsnull);
  nsresult rv = static_cast<SmsEvent*>(event.get())->Init(aEventName, false,
                                                          false, aMessage);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(true);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  rv = DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
SmsManager::Observe(nsISupports* aSubject, const char* aTopic,
                    const PRUnichar* aData)
{
  if (!strcmp(aTopic, kSmsReceivedObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-received' topic without a valid message!");
      return NS_OK;
    }

    DispatchTrustedSmsEventToSelf(RECEIVED_EVENT_NAME, message);
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsSentObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-sent' topic without a valid message!");
      return NS_OK;
    }

    DispatchTrustedSmsEventToSelf(SENT_EVENT_NAME, message);
    return NS_OK;
  }

  return NS_OK;
}

} 
} 
} 
