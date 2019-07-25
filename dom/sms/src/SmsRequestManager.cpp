




































#include "SmsRequestManager.h"
#include "nsIDOMSmsMessage.h"
#include "nsDOMEvent.h"





#define SUCCESS_EVENT_NAME NS_LITERAL_STRING("success")
#define ERROR_EVENT_NAME   NS_LITERAL_STRING("error")

namespace mozilla {
namespace dom {
namespace sms {

SmsRequestManager* SmsRequestManager::sInstance = nsnull;

void
SmsRequestManager::Init()
{
  NS_PRECONDITION(!sInstance,
                  "sInstance shouldn't be set. Did you call Init() twice?");
  sInstance = new SmsRequestManager();
}

void
SmsRequestManager::Shutdown()
{
  NS_PRECONDITION(sInstance, "sInstance should be set. Did you call Init()?");

  delete sInstance;
  sInstance = nsnull;
}

 SmsRequestManager*
SmsRequestManager::GetInstance()
{
  return sInstance;
}

PRInt32
SmsRequestManager::CreateRequest(nsPIDOMWindow* aWindow,
                                 nsIScriptContext* aScriptContext,
                                 nsIDOMMozSmsRequest** aRequest)
{
  nsCOMPtr<nsIDOMMozSmsRequest> request =
    new SmsRequest(aWindow, aScriptContext);

  PRInt32 size = mRequests.Count();

  
  for (PRInt32 i=0; i<size; ++i) {
    if (mRequests[i]) {
      continue;
    }

    mRequests.ReplaceObjectAt(request, i);
    NS_ADDREF(*aRequest = request);
    return i;
  }


  mRequests.AppendObject(request);
  NS_ADDREF(*aRequest = request);
  return size;
}

nsresult
SmsRequestManager::DispatchTrustedEventToRequest(const nsAString& aEventName,
                                                 nsIDOMMozSmsRequest* aRequest)
{
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nsnull, nsnull);
  nsresult rv = event->InitEvent(aEventName, false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  return aRequest->DispatchEvent(event, &dummy);
}

void
SmsRequestManager::NotifySuccessWithMessage(PRInt32 aRequestId,
                                            nsIDOMMozSmsMessage* aMessage)
{
  NS_ASSERTION(mRequests.Count() > aRequestId && mRequests[aRequestId],
               "Got an invalid request id or it has been already deleted!");

  
  
  SmsRequest* request = static_cast<SmsRequest*>(mRequests[aRequestId]);
  request->SetSuccess(aMessage);

  DispatchTrustedEventToRequest(SUCCESS_EVENT_NAME, request);

  mRequests.ReplaceObjectAt(nsnull, aRequestId);
}

void
SmsRequestManager::NotifyError(PRInt32 aRequestId, SmsRequest::ErrorType aError)
{
  NS_ASSERTION(mRequests.Count() > aRequestId && mRequests[aRequestId],
               "Got an invalid request id or it has been already deleted!");

  
  
  SmsRequest* request = static_cast<SmsRequest*>(mRequests[aRequestId]);
  request->SetError(aError);

  DispatchTrustedEventToRequest(ERROR_EVENT_NAME, request);

  mRequests.ReplaceObjectAt(nsnull, aRequestId);
}

void
SmsRequestManager::NotifySmsSent(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage)
{
  NotifySuccessWithMessage(aRequestId, aMessage);
}

void
SmsRequestManager::NotifySmsSendFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError)
{
  NotifyError(aRequestId, aError);
}

void
SmsRequestManager::NotifyGotSms(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage)
{
  NotifySuccessWithMessage(aRequestId, aMessage);
}

void
SmsRequestManager::NotifyGetSmsFailed(PRInt32 aRequestId,
                                      SmsRequest::ErrorType aError)
{
  NotifyError(aRequestId, aError);
}

} 
} 
} 
